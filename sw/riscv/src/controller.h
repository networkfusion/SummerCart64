#ifndef CONTROLLER_H__
#define CONTROLLER_H__


#define USB_CMD_TOKEN   (0x434D4400)
#define USB_CMP_TOKEN   (0x434D5000)
#define USB_ERR_TOKEN   (0x45525200)


typedef enum {
    USB_STATE_IDLE,
    USB_STATE_ARGS,
    USB_STATE_DATA,
    USB_STATE_RESPONSE,
    USB_STATE_N64_TO_PC,
    USB_STATE_PC_TO_N64,
} e_usb_state_t;

typedef enum {
    RTC_STATE_READ,
    RTC_STATE_WRITE,
} e_rtc_state_t;

typedef enum {
    DEBUG_WRITE_ADDRESS,
    DEBUG_WRITE_LENGTH_START,
    DEBUG_WRITE_STATUS,
    DEBUG_READ_ARG,
    DEBUG_READ_LENGTH,
    DEBUG_READ_ADDRESS_START,
} e_debug_t;

typedef enum {
    CONFIG_UPDATE_SDRAM_SWITCH,
    CONFIG_UPDATE_SDRAM_WRITABLE,
    CONFIG_UPDATE_DD_ENABLE,
    CONFIG_UPDATE_SAVE_TYPE,
    CONFIG_UPDATE_CIC_SEED,
    CONFIG_UPDATE_TV_TYPE
} e_config_update_t;

typedef enum {
    CONFIG_QUERY_STATUS,
    CONFIG_QUERY_SAVE_TYPE,
    CONFIG_QUERY_CIC_SEED,
    CONFIG_QUERY_TV_TYPE,
    CONFIG_QUERY_SAVE_OFFSET,
    CONFIG_QUERY_DD_OFFSET,
} e_config_query_t;


#endif
