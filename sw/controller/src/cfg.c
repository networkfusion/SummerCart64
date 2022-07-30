#include <stdbool.h>
#include "cfg.h"
#include "dd.h"
#include "flash.h"
#include "fpga.h"
#include "isv.h"
#include "rtc.h"
#include "usb.h"


typedef enum {
    CFG_ID_BOOTLOADER_SWITCH,
    CFG_ID_ROM_WRITE_ENABLE,
    CFG_ID_ROM_SHADOW_ENABLE,
    CFG_ID_DD_MODE,
    CFG_ID_ISV_ENABLE,
    CFG_ID_BOOT_MODE,
    CFG_ID_SAVE_TYPE,
    CFG_ID_CIC_SEED,
    CFG_ID_TV_TYPE,
    CFG_ID_FLASH_ERASE_BLOCK,
    CFG_ID_DD_DRIVE_TYPE,
    CFG_ID_DD_DISK_STATE,
} cfg_id_t;

typedef enum {
    DD_MODE_DISABLED = 0,
    DD_MODE_REGS = 1,
    DD_MODE_IPL = 2,
    DD_MODE_FULL = 3
} dd_mode_t;

typedef enum {
    BOOT_MODE_MENU_SD = 0,
    BOOT_MODE_MENU_USB = 1,
    BOOT_MODE_ROM = 2,
    BOOT_MODE_DD = 3,
    BOOT_MODE_DIRECT = 4
} boot_mode_t;

typedef enum {
    SAVE_TYPE_NONE = 0,
    SAVE_TYPE_EEPROM_4K = 1,
    SAVE_TYPE_EEPROM_16K = 2,
    SAVE_TYPE_SRAM = 3,
    SAVE_TYPE_FLASHRAM = 4,
    SAVE_TYPE_SRAM_BANKED = 5
} save_type_t;

typedef enum {
    CIC_SEED_UNKNOWN = 0xFFFF
} cic_seed_t;

typedef enum {
    TV_TYPE_PAL = 0,
    TV_TYPE_NTSC = 1,
    TV_TYPE_MPAL = 2,
    TV_TYPE_UNKNOWN = 3
} tv_type_t;

typedef enum {
    CFG_ERROR_OK = 0,
    CFG_ERROR_BAD_ADDRESS = 1,
    CFG_ERROR_BAD_CONFIG_ID = 2,
    CFG_ERROR_TIMEOUT = 3,
    CFG_ERROR_UNKNOWN_CMD = -1,
} cfg_error_t;


struct process {
    boot_mode_t boot_mode;
    save_type_t save_type;
    cic_seed_t cic_seed;
    tv_type_t tv_type;
    bool usb_output_ready;
};


static struct process p;


static void cfg_set_usb_output_ready (void) {
    p.usb_output_ready = true;
}

static bool cfg_translate_address (uint32_t *args) {
    uint32_t address = args[0];
    uint32_t length = args[1];
    if (address >= 0x10000000 && address < 0x14000000) {
        if ((address + length) <= 0x14000000) {
            args[0] = address - 0x10000000 + 0x00000000;
            return false;
        }
    }
    if (address >= 0x1FFE0000 && address < 0x1FFE2000) {
        if ((address + length) <= 0x1FFE2000) {
            args[0] = address - 0x1FFE0000 + 0x05000000;
            return false;
        }
    }
    return true;
}

static void cfg_set_error (cfg_error_t error) {
    fpga_reg_set(REG_CFG_DATA_0, error);
    fpga_reg_set(REG_CFG_DATA_1, 0);
    fpga_reg_set(REG_CFG_CMD, CFG_CMD_ERROR | CFG_CMD_DONE);
}

static void cfg_change_scr_bits (uint32_t mask, bool value) {
    if (value) {
        fpga_reg_set(REG_CFG_SCR, fpga_reg_get(REG_CFG_SCR) | mask);
    } else {
        fpga_reg_set(REG_CFG_SCR, fpga_reg_get(REG_CFG_SCR) & (~mask));
    }
}

static void cfg_set_save_type (save_type_t save_type) {
    uint32_t save_reset_mask = (
        CFG_SCR_EEPROM_16K |
        CFG_SCR_EEPROM_ENABLED |
        CFG_SCR_FLASHRAM_ENABLED |
        CFG_SCR_SRAM_BANKED |
        CFG_SCR_SRAM_ENABLED
    );

    cfg_change_scr_bits(save_reset_mask, false);

    switch (save_type) {
        case SAVE_TYPE_NONE:
            break;
        case SAVE_TYPE_EEPROM_4K:
            cfg_change_scr_bits(CFG_SCR_EEPROM_ENABLED, true);
            break;
        case SAVE_TYPE_EEPROM_16K:
            cfg_change_scr_bits(CFG_SCR_EEPROM_16K | CFG_SCR_EEPROM_ENABLED, true);
            break;
        case SAVE_TYPE_SRAM:
            cfg_change_scr_bits(CFG_SCR_SRAM_ENABLED, true);
            break;
        case SAVE_TYPE_FLASHRAM:
            cfg_change_scr_bits(CFG_SCR_FLASHRAM_ENABLED, true);
            break;
        case SAVE_TYPE_SRAM_BANKED:
            cfg_change_scr_bits(CFG_SCR_SRAM_BANKED | CFG_SCR_SRAM_ENABLED, true);
            break;
        default:
            save_type = SAVE_TYPE_NONE;
            break;
    }

    p.save_type = save_type;
}


uint32_t cfg_get_version (void) {
    return fpga_reg_get(REG_CFG_VERSION);
}

bool cfg_query (uint32_t *args) {
    uint32_t scr = fpga_reg_get(REG_CFG_SCR);

    switch (args[0]) {
        case CFG_ID_BOOTLOADER_SWITCH:
            args[1] = (scr & CFG_SCR_BOOTLOADER_ENABLED);
            break;
        case CFG_ID_ROM_WRITE_ENABLE:
            args[1] = (scr & CFG_SCR_ROM_WRITE_ENABLED);
            break;
        case CFG_ID_ROM_SHADOW_ENABLE:
            args[1] = (scr & CFG_SCR_ROM_SHADOW_ENABLED);
            break;
        case CFG_ID_DD_MODE:
            args[1] = DD_MODE_DISABLED;
            if (scr & CFG_SCR_DDIPL_ENABLED) {
                args[1] |= DD_MODE_IPL;
            }
            if (scr & CFG_SCR_DD_ENABLED) {
                args[1] |= DD_MODE_REGS;
            }
            break;
        case CFG_ID_ISV_ENABLE:
            args[1] = isv_get_enabled();
            break;
        case CFG_ID_BOOT_MODE:
            args[1] = p.boot_mode;
            break;
        case CFG_ID_SAVE_TYPE:
            args[1] = p.save_type;
            break;
        case CFG_ID_CIC_SEED:
            args[1] = p.cic_seed;
            break;
        case CFG_ID_TV_TYPE:
            args[1] = p.tv_type;
            break;
        case CFG_ID_FLASH_ERASE_BLOCK:
            args[1] = 0xFFFFFFFF;
            break;
        case CFG_ID_DD_DRIVE_TYPE:
            args[1] = dd_get_drive_type();
            break;
        case CFG_ID_DD_DISK_STATE:
            args[1] = dd_get_disk_state();
            break;
        default:
            return true;
    }

    return false;
}

bool cfg_update (uint32_t *args) {
    switch (args[0]) {
        case CFG_ID_BOOTLOADER_SWITCH:
            cfg_change_scr_bits(CFG_SCR_BOOTLOADER_ENABLED, args[1]);
            break;
        case CFG_ID_ROM_WRITE_ENABLE:
            cfg_change_scr_bits(CFG_SCR_ROM_WRITE_ENABLED, args[1]);
            break;
        case CFG_ID_ROM_SHADOW_ENABLE:
            cfg_change_scr_bits(CFG_SCR_ROM_SHADOW_ENABLED, args[1]);
            break;
        case CFG_ID_DD_MODE:
            if (args[1] == DD_MODE_DISABLED) {
                cfg_change_scr_bits(CFG_SCR_DD_ENABLED | CFG_SCR_DDIPL_ENABLED, false);
            } else if (args[1] == DD_MODE_REGS) {
                cfg_change_scr_bits(CFG_SCR_DD_ENABLED, true);
                cfg_change_scr_bits(CFG_SCR_DDIPL_ENABLED, false);
            } else if (args[1] == DD_MODE_IPL) {
                cfg_change_scr_bits(CFG_SCR_DD_ENABLED, false);
                cfg_change_scr_bits(CFG_SCR_DDIPL_ENABLED, true);
            } else {
                cfg_change_scr_bits(CFG_SCR_DD_ENABLED | CFG_SCR_DDIPL_ENABLED, true);
            }
            break;
        case CFG_ID_ISV_ENABLE:
            isv_set_enabled(args[1]);
            break;
        case CFG_ID_BOOT_MODE:
            p.boot_mode = args[1];
            cfg_change_scr_bits(CFG_SCR_BOOTLOADER_SKIP, (args[1] == BOOT_MODE_DIRECT));
            break;
        case CFG_ID_SAVE_TYPE:
            cfg_set_save_type((save_type_t) (args[1]));
            break;
        case CFG_ID_CIC_SEED:
            p.cic_seed = (cic_seed_t) (args[1] & 0xFFFF);
            break;
        case CFG_ID_TV_TYPE:
            p.tv_type = (tv_type_t) (args[1] & 0x03);
            break;
        case CFG_ID_FLASH_ERASE_BLOCK:
            flash_erase_block(args[1]);
            break;
        case CFG_ID_DD_DRIVE_TYPE:
            dd_set_drive_type(args[1]);
            break;
        case CFG_ID_DD_DISK_STATE:
            dd_set_disk_state(args[1]);
            break;
        default:
            return true;
    }

    return false;
}

void cfg_get_time (uint32_t *args) {
    rtc_time_t t;
    rtc_get_time(&t);
    args[0] = ((t.hour << 16) | (t.minute << 8) | t.second);
    args[1] = ((t.weekday << 24) | (t.year << 16) | (t.month << 8) | t.day);
}

void cfg_set_time (uint32_t *args) {
    rtc_time_t t;
    t.second = (args[0] & 0xFF);
    t.minute = ((args[0] >> 8) & 0xFF);
    t.hour = ((args[0] >> 16) & 0xFF);
    t.weekday = ((args[1] >> 24) & 0xFF);
    t.day = (args[1] & 0xFF);
    t.month = ((args[1] >> 8) & 0xFF);
    t.year = ((args[1] >> 16) & 0xFF);
    rtc_set_time(&t);
}

void cfg_init (void) {
    fpga_reg_set(REG_CFG_SCR, 0);
    cfg_set_save_type(SAVE_TYPE_NONE);

    p.cic_seed = CIC_SEED_UNKNOWN;
    p.tv_type = TV_TYPE_UNKNOWN;
    p.boot_mode = BOOT_MODE_MENU_SD;
    p.usb_output_ready = true;
}

void cfg_process (void) {
    uint32_t args[2];
    usb_tx_info_t packet_info;

    if (fpga_reg_get(REG_STATUS) & STATUS_CFG_PENDING) {
        args[0] = fpga_reg_get(REG_CFG_DATA_0);
        args[1] = fpga_reg_get(REG_CFG_DATA_1);
        char cmd = (char) fpga_reg_get(REG_CFG_CMD);

        switch (cmd) {
            case 'v':
                args[0] = cfg_get_version();
                break;

            case 'c':
                if (cfg_query(args)) {
                    cfg_set_error(CFG_ERROR_BAD_CONFIG_ID);
                    return;
                }
                break;

            case 'C':
                if (cfg_update(args)) {
                    cfg_set_error(CFG_ERROR_BAD_CONFIG_ID);
                    return;
                }
                break;

            case 't':
                cfg_get_time(args);
                break;

            case 'T':
                cfg_set_time(args);
                break;

            case 'm':
                if (cfg_translate_address(args)) {
                    cfg_set_error(CFG_ERROR_BAD_ADDRESS);
                    return;
                }
                if (!usb_prepare_read(args)) {
                    return;
                }
                break;

            case 'M':
                if (cfg_translate_address(args)) {
                    cfg_set_error(CFG_ERROR_BAD_ADDRESS);
                    return;
                }
                usb_create_packet(&packet_info, PACKET_CMD_USB_OUTPUT);
                packet_info.dma_length = args[1];
                packet_info.dma_address = args[0];
                packet_info.done_callback = cfg_set_usb_output_ready;
                if (usb_enqueue_packet(&packet_info)) {
                    p.usb_output_ready = false;
                } else {
                    return;
                }
                break;

            case 'u':
                usb_get_read_info(args);
                break;

            case 'U':
                args[0] = p.usb_output_ready ? 1 : 0;
                break;

            default:
                cfg_set_error(CFG_ERROR_UNKNOWN_CMD);
                return;
        }

        fpga_reg_set(REG_CFG_DATA_0, args[0]);
        fpga_reg_set(REG_CFG_DATA_1, args[1]);
        fpga_reg_set(REG_CFG_CMD, CFG_CMD_DONE);
    }
}
