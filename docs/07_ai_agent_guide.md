# AI Agent Guide for SummerCart64 Repository

**Last Updated**: 2026-08-03 (verified by full build on this date)  
**Purpose**: Comprehensive guide for AI agents to understand the SummerCart64 project architecture, build system, and development workflow

---

## Table of Contents

- [Project Overview](#project-overview)
- [Repository Structure](#repository-structure)
- [Technical Architecture](#technical-architecture)
- [Build System](#build-system)
- [Key Dependencies & Requirements](#key-dependencies--requirements)
- [Development Workflow](#development-workflow)
- [Important Constraints](#important-constraints)

---

## Project Overview

**SummerCart64** is a fully open-source N64 flashcart project combining:
- **Hardware**: PCB design with FPGA, microcontroller, and supporting components
- **Firmware**: FPGA bitstream, bootloader, and controller firmware
- **Software**: CLI deployer tool for configuration and file management
- **Web**: GitHub Pages website for project information

### Key Characteristics
- Active fork maintained by `networkfusion` GitHub organization (origin)
- Upstream repository: `Polprzewodnikowy/SummerCart64`
- Build system based on Docker containers for reproducible builds
- Requires proprietary Lattice Diamond license for FPGA compilation
- Multi-language codebase: Verilog/SystemVerilog, C/C++, Rust, Python, Bash

---

## Repository Structure

```
SummerCollection/
├── .github/workflows/
│   └── build.yml                    # GitHub Actions CI/CD pipeline
├── docs/                            # Documentation
│   ├── 00_quick_startup_guide.md
│   ├── 01_memory_map.md
│   ├── 02_n64_commands.md
│   ├── 03_usb_interface.md
│   ├── 04_config_options.md
│   ├── 05_fw_and_sw_info.md
│   ├── 06_build_guide.md
│   └── 07_ai_agent_guide.md         # This file
│
├── fw/                              # Firmware
│   ├── ftdi/                        # FTDI USB-UART chip configuration
│   │   └── ft232h_config.xml        # Template for FT232H EEPROM
│   ├── project/
│   │   └── lcmxo2/                  # FPGA project (Lattice Diamond)
│   │       ├── build.sh
│   │       ├── build.tcl
│   │       ├── debug.sty            # Debug build style
│   │       ├── release.sty          # Release build style
│   │       ├── sc64.ldf             # FPGA constraints
│   │       ├── sc64.lpf             # Logic placement file
│   │       └── license.dat          # Lattice Diamond license (REQUIRED)
│   ├── rtl/                         # Register Transfer Level (RTL) hardware design
│   │   ├── top.sv                   # Top-level module
│   │   ├── fifo/                    # FIFO modules
│   │   ├── mcu/                     # Microcontroller interface
│   │   ├── memory/                  # Memory management (SDRAM, FLASH, DMA)
│   │   ├── n64/                     # N64 interface logic
│   │   ├── sd/                      # SD card interface
│   │   ├── serv/                    # SERV RISC-V CPU core
│   │   ├── usb/                     # USB interface (FT1248)
│   │   └── vendor/                  # Vendor-specific modules
│   └── tests/                       # Hardware simulation tests
│       ├── benches/                 # Testbenches
│       └── mocks/                   # Mock components for simulation
│
├── hw/                              # Hardware design
│   ├── pcb/                         # KiCad PCB project
│   │   ├── sc64v2.kicad_pro         # Project file
│   │   ├── sc64v2.kicad_sch         # Schematic
│   │   ├── sc64v2.kicad_pcb         # PCB layout
│   │   ├── sc64v2_bom.html          # Bill of Materials (interactive)
│   │   └── LICENSE
│   └── shell/                       # 3D printable enclosure
│       ├── sc64_shell_front.stl
│       └── sc64_shell_back.stl
│
├── sw/                              # Software
│   ├── bootloader/                  # N64 bootloader (C/ASM)
│   │   ├── Makefile
│   │   ├── N64.ld                   # Linker script
│   │   ├── src/
│   │   ├── assets/
│   │   └── tools/
│   ├── controller/                  # Microcontroller firmware (C)
│   │   ├── build.sh
│   │   ├── app.ld / app.mk          # Application linker/makefile
│   │   ├── loader.ld / loader.mk    # Loader linker/makefile
│   │   ├── primer.ld / primer.mk    # Primer linker/makefile
│   │   ├── inc/
│   │   └── src/
│   ├── cic/                         # CIC emulation (C/ASM)
│   │   ├── build.sh
│   │   ├── cic.c / cic.h
│   │   ├── startup.S
│   │   ├── cic.ld
│   │   └── convert.py
│   ├── deployer/                    # CLI tool (Rust, PC-side)
│   │   ├── Cargo.toml
│   │   ├── build.rs
│   │   ├── src/
│   │   └── data/
│   └── tools/                       # Python utility scripts
│       ├── primer.py                # Initial programming script
│       ├── update.py                # Update utility
│       └── requirements.txt         # Python dependencies
│
├── web/                             # GitHub Pages website
│   ├── index.html
│   ├── features.html
│   ├── faq.html
│   ├── bom.html
│   ├── styles.css
│   └── script.js
│
├── assets/                          # Images, diagrams, logos
├── .vscode/                         # VS Code workspace settings
├── build.sh                         # Main build orchestration script
├── docker_build.sh                  # Docker build wrapper
├── LICENSE
└── README.md
```

---

## Technical Architecture

### Hardware Components

1. **Microcontroller**: Connected to N64 console and manages system operations
2. **FPGA (Lattice MachXO2)**: Handles digital logic for:
   - N64 cartridge bus interface (ROM, save data, CIC emulation)
   - SD card interface (~23.8 MiB/s)
   - USB-UART interface (~23.8 MiB/s, uses FT232H chip)
   - 64DD add-on emulation
   - Real-time clock (RTC) with battery backup
3. **Memory Hierarchy**:
   - 64 MiB SDRAM (game and save data)
   - 16 MiB FLASH (bootloader, extended game data)
   - 8 KiB on-chip FPGA buffer (general use)
4. **Support Chips**:
   - FT232H: USB-to-UART bridge (FTDI chip requiring EEPROM configuration)
   - Various interface chips for SD, USB, N64 connectivity

### Firmware Components

1. **FPGA Bitstream** (`fw/project/lcmxo2/`):
   - Compiled from RTL modules (Verilog/SystemVerilog)
   - Generated by Lattice Diamond (requires license)
   - Output: `sc64-firmware-{version}.bin`

2. **Bootloader** (`sw/bootloader/`):
   - Minimal N64 bootrom replacement
   - Handles menu loading from SD card
   - Supports IPL3 register spoofing
   - Compiled to binary, included in firmware package

3. **Controller Firmware** (`sw/controller/`):
   - Microcontroller application code
   - Multiple build profiles: app, loader, primer
   - Handles USB communication, save management
   - Compiled to binary, included in firmware package

4. **CIC Emulation** (`sw/cic/`):
   - UltraCIC_C emulation for region switching
   - Compiled separately, integrated into controller firmware

### Software (PC-side)

1. **Deployer** (`sw/deployer/`):
   - Rust-based CLI tool for configuration and file management
   - Cross-platform (Windows, Linux, macOS, 32-bit variants)
   - Supports game/save upload, feature control, debug terminal
   - Uses USB interface (FT1248 protocol over FT232H)

2. **Utility Scripts** (`sw/tools/`):
   - `primer.py`: Initial FPGA/microcontroller programming via UART
   - `update.py`: Firmware update utility

---

## Build System

### Build Orchestration

**Main Script**: `build.sh` - Orchestrates all build stages, called by docker container

```bash
# Docker wrapper (runs build inside container)
./docker_build.sh release --force-clean
```

> **Windows users**: `docker_build.sh` cannot be called directly from PowerShell or via `wsl --` because those shells cannot connect to the Docker Desktop daemon. See [Building on Windows](#building-on-windows) for the correct approach.

**Build Targets** (defined in build.sh):
- `bootloader` - Compile N64 bootloader
- `controller` - Compile microcontroller firmware (includes CIC emulation)
- `cic` - Compile CIC emulation separately
- `fpga` - Compile FPGA bitstream (requires Lattice Diamond license)
- `update` - Bundle firmware into update package
- `release` - Full build + package all artifacts

### Docker Build Process

**Container Image**: `ghcr.io/polprzewodnikowy/sc64env:v1.10`

**Key Characteristics**:
- Runs on Linux/x86_64 architecture
- Contains pre-installed build tools (arm-gcc, Lattice Diamond, Python)
- Mounts `/workdir` to project root
- Mounts `/flexlm/license.dat` to firmware license file
- Runs as non-root (preserves user/group IDs)

**Important Environment Variables**:
- `SC64_VERSION`: Version string injected into firmware metadata
- `MAC_ADDRESS`: MAC address linked to Lattice Diamond license (default: `F8:12:34:56:78:90`)
- `USER_FLAGS`: Custom compiler flags passed to build steps

### Build Output

**Primary Outputs**:
- `sc64-firmware-{version}.bin` - Combined FPGA + bootloader + controller firmware
- `sc64-extra-{version}.zip` - Release package with:
  - Firmware binary
  - Documentation
  - PCB manufacturing files (KiCad project, BOM, 3D shell files)
  - Programming tools (`primer.py`, `requirements.txt`)
  - Hardware diagrams and configuration templates

**Build Artifacts**:
- `fw/project/lcmxo2/impl1/` - FPGA synthesis/place&route results
- Software object files in respective source directories
- Intermediate build artifacts in `build/` directories

---

## Key Dependencies & Requirements

### Hardware Build Requirements

| Component | Requirement | Details |
|-----------|-----------|---------|
| **Lattice Diamond** | Proprietary license | 1-year free license available from Lattice |
| **Lattice License File** | `fw/project/lcmxo2/license.dat` | Required for FPGA compilation |
| **Mac Address** | Linked to license | Passed as `MAC_ADDRESS` env var |
| **Docker** | Docker Engine | For reproducible builds (WSL2 on Windows) |

### Software Build Requirements

| Component | Language | Key Tools | Notes |
|-----------|----------|-----------|-------|
| **FPGA Bitstream** | Verilog/SystemVerilog | Lattice Diamond 3.13 | Proprietary tool, no Vivado |
| **Bootloader** | C, ARM ASM | arm-none-eabi-gcc, arm-none-eabi-newlib | GCC-based ARM toolchain |
| **Controller** | C | arm-none-eabi-gcc | GCC-based ARM toolchain |
| **CIC Emulation** | C, ARM ASM | arm-none-eabi-gcc | Part of controller build |
| **Deployer** | Rust | Cargo, rustc | Cargo package manager |
| **Tools** | Python | Python 3, pip | `requirements.txt` specifies: pyserial, other USB libs |

### Build Dependencies (in Docker)

- `arm-none-eabi-gcc`: GCC ARM toolchain
- `Lattice Diamond 3.11.x or newer`: FPGA synthesis/place&route
- `Rust toolchain`: For deployer compilation
- `Python 3`: For utility scripts and build helpers
- `make`: Build orchestration
- Standard Unix tools: bash, sed, awk, etc.

---

## Development Workflow

### Typical Development Tasks

#### 1. Modifying FPGA Logic (Verilog/SystemVerilog)

**Files**: `fw/rtl/**/*.sv`

Steps:
1. Edit module in `fw/rtl/` hierarchy
2. Update top-level constraints if needed (`fw/project/lcmxo2/sc64.lpf`)
3. Run `./docker_build.sh fpga` to compile
4. Check output in `fw/project/lcmxo2/impl1/` for timing/resource utilization
5. If timing closed: Commit changes, otherwise iterate on design

**Related Modules**:
- N64 interface: `fw/rtl/n64/*.sv`
- Memory management: `fw/rtl/memory/*.sv`
- USB/SD interfaces: `fw/rtl/usb/*.sv`, `fw/rtl/sd/*.sv`

#### 2. Modifying Microcontroller Firmware (C)

**Files**: `sw/controller/src/**/*.c`, `sw/controller/inc/**/*.h`

Steps:
1. Edit source files in `sw/controller/`
2. Run `./docker_build.sh controller` to compile
3. Firmware will be automatically combined into `sc64-firmware-{version}.bin`
4. Test via deployer tool or hardware

#### 3. Updating Bootloader (C/ASM)

**Files**: `sw/bootloader/src/**/*`

Steps:
1. Edit bootloader source
2. Run `./docker_build.sh bootloader`
3. Changes included in next full build

#### 4. Building PC-Side Tools (Rust)

**Files**: `sw/deployer/src/**/*.rs`

Steps:
1. Edit Rust sources
2. Run `cargo build -r` inside `sw/deployer/`
3. Or use CI/CD pipeline for multi-platform builds

#### 5. Full Release Build

**Linux/macOS**:
```bash
SC64_VERSION="v3.0" MAC_ADDRESS="AB:00:00:00:00:00" ./docker_build.sh release --force-clean
```

**Windows (PowerShell — only method that works)**:
```powershell
$macAddress = "AB:00:00:00:00:00"   # from HOSTID= field in license.dat
$licenseFile = "$(Get-Location)\fw\project\lcmxo2\license.dat"
$workDir = "$(Get-Location)"
$image = "ghcr.io/polprzewodnikowy/sc64env:v1.10"

docker run --rm --mac-address $macAddress `
    -v "${licenseFile}:/flexlm/license.dat" `
    -v "${workDir}:/workdir" `
    -e SC64_VERSION="v3.0" `
    --platform linux/x86_64 -w /workdir `
    $image ./build.sh release
```

Output: `sc64-extra-v3.0.zip` with all artifacts

### CI/CD Pipeline (.github/workflows/build.yml)

**Triggers**:
- Push to main branch
- Pull requests to main
- Release creation
- Manual workflow dispatch

**Jobs**:

1. **build-firmware**: Builds FPGA + bootloader + controller
   - Requires Lattice license (from secrets)
   - Produces: `sc64-firmware-{version}.bin`, `sc64-extra-{version}.zip`

2. **build-deployer**: Cross-platform deployer builds
   - Matrix strategy: Windows (32/64-bit), Linux, macOS
   - Produces: Platform-specific executables

3. **publish-website**: Updates GitHub Pages
   - Copies BOM from PCB project
   - Publishes web/ directory

---

## Important Constraints

### License Constraints

1. **Lattice Diamond License**
   - Required for FPGA compilation (no free tier for commercial redistribution)
   - Available as 1-year **node-locked** personal license (free)
   - Request from https://www.latticesemi.com/Support/Licensing — select device family **MachXO2** specifically
   - License is locked to a specific physical MAC address (must match a real NIC, not a virtual adapter)
   - Modern license format embeds MAC as `HOSTID=<hex>` within each `FEATURE` line (not a `SERVER` line)
   - Extract MAC: `Select-String -Path license.dat -Pattern 'HOSTID=([0-9a-f]+)' | Select -First 1`
   - File must be placed at `fw/project/lcmxo2/license.dat`

2. **Third-party Licenses**
   - SERV RISC-V core: License included in `fw/rtl/serv/LICENSE`
   - KiCad project: Commercial use may require licensing review
   - Various open-source components (see LICENSE files in respective folders)

### Platform Constraints

1. **FPGA Compilation**
   - Only available inside Docker container with Lattice Diamond
   - Linux/x86_64 platform requirement
   - On Windows: must invoke `docker run` directly from PowerShell — `wsl -- ./docker_build.sh` and `bash -c './docker_build.sh'` both fail because those shells cannot reach the Docker Desktop daemon
   - Verify Docker context: `docker context use desktop-linux`

2. **Line Endings**
   - Repository files are stored as CRLF (Windows-native project)
   - Shell scripts and Makefiles have `eol=lf` in `.gitattributes` so git serves them as LF on all platforms
   - Without this, bash scripts produce `/bin/bash^M: bad interpreter` inside Docker

3. **Initial Programming**
   - Requires hardware: USB-to-UART adapter (3.3V)
   - Windows 10+ or Linux
   - FTDI drivers required
   - FT_PROG software required for EEPROM configuration

### Build Constraints

1. **Version Strings**
   - Git information (branch, tag, SHA, message) injected during build
   - `SC64_VERSION` environment variable controls package naming
   - Build requires git repository access (`git` commands in build.sh)

2. **Resource Requirements**
   - FPGA compilation takes ~26 minutes (verified); Docker image is ~2.3 GB on first pull
   - Network access required for fetching docker image first run

### Performance Notes

- SD card interface: ~23.8 MiB/s peak
- USB interface: ~23.8 MiB/s peak
- FPGA frequency: 102.934 MHz achieved (100 MHz required) — verified 2026-08-03
- FPGA resource utilization (verified): SLICEs 92%, Block RAMs 100%, LUT4s 72% — design is near capacity
- Check `fw/project/lcmxo2/impl1/fpga_max_frequency.txt` after build

---

## Additional Resources

- **Official Website**: https://summercart64.dev
- **Hardware Diagrams**: See `assets/sc64_block_diagram.svg`
- **N64 Interface Details**: `docs/02_n64_commands.md`
- **USB Protocol**: `docs/03_usb_interface.md`
- **Memory Map**: `docs/01_memory_map.md`
- **Video Build Guide**: https://www.youtube.com/watch?v=t6hyCFpwqz8

---

## Notes for AI Agents

### When Modifying Code

- **Verilog/SystemVerilog files** in `fw/rtl/` require full `docker_build.sh fpga` cycle
- **C files** need to match their module's naming conventions (e.g., `fw/rtl/n64/n64_top.sv` ↔️ `sw/controller/src/n64.c`)
- **Python/Rust code** for PC tools can be tested independently without docker
- Always check git version info is embedded correctly for release builds

### When Troubleshooting Builds

- **License errors**: Verify `MAC_ADDRESS` matches the `HOSTID=` field in `license.dat` (not the `SERVER` line — modern licenses don't have one). Format must be colon-separated (`AA:BB:CC:DD:EE:FF`), not hyphens.
- **Wrong MAC**: License is node-locked to a physical NIC MAC. Use `Get-NetAdapter | Where Status -eq Up` (Windows) to find physical adapters. Virtual/WSL adapters won't match.
- **Docker errors on Windows**: Do NOT use `wsl -- ./docker_build.sh` or `bash -c`. Run `docker run` directly in PowerShell. Check `docker context use desktop-linux`.
- **`/bin/bash^M: bad interpreter`**: Shell scripts have CRLF line endings. `.gitattributes` now prevents this on fresh checkouts, but if it happens: `bash -c "sed -i 's/\r$//' build.sh docker_build.sh"`
- **Timing violations**: FPGA design is at 92% SLICE utilization — any significant RTL additions may require timing-driven optimisation.
- **Git information issues**: Ensure repository has proper tags/commits for version strings.

### Repository Characteristics

- **Active Development**: Regular PRs and releases
- **Upstream Tracking**: May need to sync with `upstream/main` periodically
- **Issue Tracking**: GitHub Issues for bug reports and feature requests
- **Documentation**: Well-organized docs/ folder with comprehensive guides

