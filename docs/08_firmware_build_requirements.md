# Firmware Build Requirements and Troubleshooting Guide

**Last Updated**: 2026-08-03  
**Audience**: Developers building SummerCart64 firmware locally

---

## Table of Contents

- [Quick Start](#quick-start)
- [Prerequisites](#prerequisites)
- [Lattice Diamond License Setup](#lattice-diamond-license-setup)
- [Build Environment](#build-environment)
- [Building Firmware](#building-firmware)
- [Troubleshooting](#troubleshooting)
- [CI/CD Integration](#cicd-integration)

---

## Quick Start

### For Local Development (Windows with WSL2)

```bash
# 1. Ensure WSL2 is running
wsl -l -v

# 2. Install Docker (if not already installed)
# From Microsoft Store or: https://www.docker.com/products/docker-desktop

# 3. Obtain Lattice Diamond license
# - Visit: https://www.latticesemi.com/Support/Licensing
# - Request 1-year free personal license
# - Receive license.dat file and associated MAC address

# 4. Place license file
cp ~/Downloads/license.dat fw/project/lcmxo2/license.dat

# 5. Build specific component (e.g., FPGA only)
MAC_ADDRESS="AB:00:00:00:00:00" ./docker_build.sh fpga

# 6. Build complete firmware
SC64_VERSION="dev" MAC_ADDRESS="AB:00:00:00:00:00" ./docker_build.sh release --force-clean
```

### For Building Only PC Tools (No License Required)

```bash
cd sw/deployer
cargo build --release

# Output: target/release/sc64deployer or sc64deployer.exe
```

---

## Prerequisites

### System Requirements

| Requirement | Minimum | Recommended | Notes |
|-------------|---------|-------------|-------|
| **OS** | Windows 10 / Ubuntu 18.04 | Windows 11 / Ubuntu 22.04 | WSL2 on Windows for full build |
| **RAM** | 8 GB | 16 GB | FPGA synthesis needs memory |
| **Disk Space** | 50 GB | 100 GB | Docker image + build artifacts |
| **Docker** | 20.10 | Latest | Desktop or daemon edition |
| **Git** | Any version | Latest | For version string injection |

### Software to Install

#### On Windows (for Docker builds)

1. **Docker Desktop for Windows** (includes WSL2)
   - Download: https://www.docker.com/products/docker-desktop
   - Enable WSL2 backend in settings
   - Run: `docker run hello-world` to verify

2. **Git for Windows** (optional, for local repo)
   - Download: https://git-scm.com/download/win
   - Or use Windows Subsystem for Git (built-in)

3. **WSL2 Linux Distribution**
   - Already included with Docker Desktop
   - Or install manually: `wsl --install`

#### On Linux (native builds possible but not officially supported)

1. **Docker Engine**
   ```bash
   sudo apt-get update
   sudo apt-get install docker.io docker-compose
   sudo usermod -aG docker $USER
   ```

2. **Git**
   ```bash
   sudo apt-get install git
   ```

3. **Rust toolchain** (for deployer builds)
   ```bash
   curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
   ```

#### On macOS (native support for deployer, Docker for full build)

1. **Docker Desktop for Mac**
   - Download: https://www.docker.com/products/docker-desktop

2. **Xcode Command Line Tools**
   ```bash
   xcode-select --install
   ```

3. **Homebrew packages**
   ```bash
   brew install git rust
   ```

---

## Lattice Diamond License Setup

### Obtaining a License

**Step 1**: Request personal license
- Visit: https://www.latticesemi.com/Support/Licensing
- Select "Personal License" or "Free Evaluation"
- Fill out the form with your name and email
- Select "MachXO2" device family
- Receive email with license file and details

**Step 2**: License file contents
```
License file (license.dat): Base64-encoded or plain text license
MAC Address: Format like "AB:00:00:00:00:00" or "AB-00-00-00-00-00"
License duration: 1 year from issue date
Device families: MachXO2 (sometimes FPGA or broader)
```

### Installing the License

#### Method 1: File Placement (For Docker Builds)

1. Download `license.dat` from Lattice
2. Copy to project:
   ```bash
   cp ~/Downloads/license.dat fw/project/lcmxo2/license.dat
   ```
3. Note the MAC address from your license email
4. Use MAC address in docker_build.sh:
   ```bash
   MAC_ADDRESS="AB:00:00:00:00:00" ./docker_build.sh release
   ```

#### Method 2: Environment Variable (Docker)

If your MAC address is embedded in license or known:
```bash
export MAC_ADDRESS="AB:00:00:00:00:00"
./docker_build.sh release
```

#### Method 3: Via GitHub Actions Secrets (CI/CD)

For automated builds (already configured in `.github/workflows/build.yml`):
```yaml
env:
  MAC_ADDRESS: ${{ secrets.LATTICE_DIAMOND_MAC_ADDRESS }}
  LICENSE: ${{ secrets.LATTICE_DIAMOND_LICENSE_BASE64 }}
```

Repository secrets are configured by maintainers.

### Troubleshooting License Issues

**Error**: `Could not find FLEXlm license server or license file`

- Verify file exists: `test -f fw/project/lcmxo2/license.dat && echo "Found"`
- Check file is readable: `chmod 644 fw/project/lcmxo2/license.dat`
- Verify file format (should be text or base64, not corrupted)
- Confirm MAC address matches: `MAC_ADDRESS="<YOUR_MAC>" docker_build.sh fpga`

**Error**: `License for device "MachXO2" not found`

- License may not include MachXO2 family
- Request new license specifically for MachXO2
- Check license expiration date hasn't passed

**Error**: `MAC address mismatch`

- Lattice licenses are locked to specific MAC addresses
- Use exactly: `MAC_ADDRESS="AB:00:00:00:00:00"` (format with colons)
- Do NOT use hyphens format: "AB-00-00-00-00-00"

---

## Build Environment

### Docker Container Details

**Image**: `ghcr.io/polprzewodnikowy/sc64env:v1.10`

**Pre-installed Tools**:
- Lattice Diamond (with license support)
- GCC ARM toolchain: `arm-none-eabi-gcc`
- Rust toolchain (for deployer builds)
- Python 3 with common packages
- GNU Make and Unix utilities
- Git

**Image Size**: ~5 GB (first pull may take 5-10 minutes)

**Update Image**:
```bash
docker pull ghcr.io/polprzewodnikowy/sc64env:v1.10
```

### Docker Build Process Flow

```
./docker_build.sh [command]
    Ôåô
docker_build.sh invokes docker run with:
    - Image: sc64env:v1.10
    - Mount points: license.dat, project directory
    - MAC_ADDRESS env var
    - User/group ID preserved
    Ôåô
Docker container runs ./build.sh [command]
    Ôåô
build.sh orchestrates:
    1. Git info extraction (branch, tag, SHA)
    2. Bootloader compilation (sw/bootloader)
    3. CIC emulation (sw/cic)
    4. Controller firmware (sw/controller)
    5. FPGA bitstream (fw/project/lcmxo2) - requires license
    6. Package generation (if 'release' target)
    Ôåô
Outputs written to host filesystem
```

### Environment Variables

**Required for FPGA builds**:
- `MAC_ADDRESS`: MAC address linked to Lattice license (format: `AB:00:00:00:00:00`)

**Optional**:
- `SC64_VERSION`: Version string for packaging (default: "")
- `USER_FLAGS`: Custom compiler flags passed to bootloader/controller builds
- `FORCE_CLEAN`: Set to "true" in build.sh to clean before building

**Docker-specific** (handled by docker_build.sh):
- `DOCKER_OPTIONS`: Set to `-it` for interactive terminal (auto-detected)
- `BUILDER_IMAGE`: Docker image to use
- `BUILDER_PLATFORM`: Docker build platform (linux/x86_64)

---

## Building Firmware

### Build Targets

```bash
# Build specific components (each can be run independently)
./docker_build.sh bootloader      # N64 bootloader only (~1 min)
./docker_build.sh controller      # Microcontroller firmware (~2 min)
./docker_build.sh cic             # CIC emulation (~1 min)
./docker_build.sh fpga            # FPGA bitstream only (~10-15 min, requires license)
./docker_build.sh update          # Package update (~1 min)
./docker_build.sh release         # Full build + packaging (~20 min, requires license)
```

### Common Build Commands

**Development build** (incremental, keeps intermediate files):
```bash
MAC_ADDRESS="AB:00:00:00:00:00" ./docker_build.sh fpga
```

**Release build** (clean build, optimized):
```bash
SC64_VERSION="v3.0" MAC_ADDRESS="AB:00:00:00:00:00" ./docker_build.sh release --force-clean
```

**Building without FPGA** (no license needed):
```bash
./docker_build.sh bootloader
./docker_build.sh controller
./docker_build.sh cic
```

**PC tools only** (outside docker, no license needed):
```bash
cd sw/deployer
cargo build --release

# For 32-bit Windows:
cargo build --release --target=i686-pc-windows-msvc

# For Linux:
cargo build --release
```

### Build Output Artifacts

**After `./docker_build.sh release --force-clean`**:

```
SummerCollection/
Ôö£ÔöÇÔöÇ sc64-firmware-v3.0.bin          # Main firmware (FPGA + bootloader + controller)
Ôö£ÔöÇÔöÇ sc64-extra-v3.0.zip             # Release package with:
Ôöé   Ôö£ÔöÇÔöÇ sc64-firmware-v3.0.bin      # Firmware binary
Ôöé   Ôö£ÔöÇÔöÇ primer.py                   # Initial programming script
Ôöé   Ôö£ÔöÇÔöÇ requirements.txt            # Python dependencies for primer.py
Ôöé   Ôö£ÔöÇÔöÇ ft232h_config.xml           # FTDI chip template
Ôöé   Ôö£ÔöÇÔöÇ docs/                       # Documentation
Ôöé   Ôö£ÔöÇÔöÇ hw/pcb/                     # KiCad project files
Ôöé   Ôö£ÔöÇÔöÇ hw/shell/                   # 3D shell .stl files
Ôöé   Ôö£ÔöÇÔöÇ README.md
Ôöé   ÔööÔöÇÔöÇ LICENSE
ÔööÔöÇÔöÇ fw/project/lcmxo2/impl1/        # FPGA build artifacts
    Ôö£ÔöÇÔöÇ fpga_sc64.bit               # FPGA bitstream
    Ôö£ÔöÇÔöÇ fpga_max_frequency.txt      # Achievable clock frequency
    ÔööÔöÇÔöÇ [synthesis/place&route files]
```

**For PC tools**:
```
sw/deployer/target/release/
Ôö£ÔöÇÔöÇ sc64deployer                    # Linux/macOS executable
Ôö£ÔöÇÔöÇ sc64deployer.exe                # Windows executable
ÔööÔöÇÔöÇ [library files]
```

### Build Performance Benchmarks

| Target | Time | Size | Notes |
|--------|------|------|-------|
| bootloader | ~1 min | 64 KiB | Fast |
| controller | ~2 min | 256 KiB | Moderate |
| cic | ~1 min | 8 KiB | Fast |
| fpga | ~10-15 min | ~1 MiB | Slow, license required |
| deployer (release) | ~2-5 min | 15-30 MiB | Platform-dependent |
| Full release | ~20 min | Combined | First run includes docker pull |

**First Run**: Add 5-10 minutes for docker image download (~5 GB)

---

## Troubleshooting

### License Issues

**Problem**: `License for product "LatticeECP3" or similar not found`

**Solution**:
1. Verify license is for MachXO2 device family
2. Request new license from https://www.latticesemi.com/Support/Licensing
3. Check license file is in correct location: `fw/project/lcmxo2/license.dat`
4. Verify MAC address format: `AB:00:00:00:00:00` (colons, not hyphens)

**Problem**: `MAC address mismatch` or `Cannot find license for this MAC address`

**Solution**:
```bash
# Verify MAC address from license file
grep -i "MAC\|HOST" fw/project/lcmxo2/license.dat

# Use exact MAC address in build
MAC_ADDRESS="<extracted-mac-address>" ./docker_build.sh fpga
```

### Docker Issues

**Problem**: `Docker daemon not running`

**Solution**:
- Windows: Start Docker Desktop (check taskbar icon)
- Linux: `sudo systemctl start docker`
- macOS: Start Docker from Applications

**Problem**: `Cannot connect to Docker daemon`

**Solution**:
```bash
# Verify docker is installed and running
docker ps

# If not running:
# Windows: Start Docker Desktop
# Linux: sudo usermod -aG docker $USER && newgrp docker
# macOS: Open Applications/Docker.app
```

**Problem**: `Image pull timeout or network error`

**Solution**:
```bash
# Retry pull with longer timeout
docker pull --retry-max=3 ghcr.io/polprzewodnikowy/sc64env:v1.10

# Check docker system
docker system df
docker system prune -a  # Clean old images if disk space is issue
```

### Build Failures

**Problem**: `make: arm-none-eabi-gcc: not found`

**Cause**: Toolchain not available in Docker image  
**Solution**: 
- Verify Docker image is correct: `docker image ls | grep sc64env`
- Pull latest: `docker pull ghcr.io/polprzewodnikowy/sc64env:v1.10`

**Problem**: `Permission denied` on build.sh or scripts

**Solution**:
```bash
chmod +x build.sh docker_build.sh
chmod +x sw/bootloader/Makefile
chmod +x sw/controller/build.sh
chmod +x sw/cic/build.sh
```

**Problem**: `Git information not embedded in firmware`

**Solution**:
- Ensure repository has git history: `git log --oneline -5`
- Pass explicit version: `SC64_VERSION="v3.0" ./docker_build.sh release`
- Check git is available in container (should be automatic)

**Problem**: FPGA timing violations: `Failed to close timing`

**Solution**:
- Try release build style: Already configured in `fw/project/lcmxo2/release.sty`
- Review `fw/project/lcmxo2/impl1/fpga_max_frequency.txt` for achieved frequency
- May need RTL optimization or incremental synthesis attempts

### File System Issues

**Problem**: `Permission denied` writing to output files

**Solution** (Windows):
```bash
# Run PowerShell as Administrator if needed
# Or ensure output directory is writable:
icacls "$(pwd)" /grant:r "$env:USERNAME`:F" /T
```

**Problem**: `Disk space full` during docker build

**Solution**:
```bash
# Clean up docker artifacts
docker system prune -a

# Remove old builds
rm -rf fw/project/lcmxo2/impl1/

# Check disk usage
df -h  # Linux/macOS
Get-Volume  # Windows PowerShell
```

### Git Issues

**Problem**: `Git not found` in build

**Solution**:
- Ensure `git` is installed: `git --version`
- Verify repository has git history: `git log -1`
- Git is pre-installed in Docker image

**Problem**: `Unsafe directory` error in git commands

**Solution** (already handled by build.sh):
```bash
# If manual git commands needed:
git -c safe.directory="$(pwd)" describe --tags
```

---

## CI/CD Integration

### GitHub Actions Workflow

**File**: `.github/workflows/build.yml`

**Triggers**:
- Push to `main` branch
- Pull requests to `main`
- Release creation (`release` event)
- Manual dispatch (`workflow_dispatch`)

### Secrets Configuration

Required repository secrets (for CI/CD):

1. **`LATTICE_DIAMOND_LICENSE_BASE64`**
   ```bash
   # Convert license file to base64:
   cat fw/project/lcmxo2/license.dat | base64 > license_base64.txt
   
   # Copy content to GitHub Secrets
   # Settings ÔåÆ Secrets and variables ÔåÆ Actions ÔåÆ New repository secret
   # Name: LATTICE_DIAMOND_LICENSE_BASE64
   # Value: [paste base64 content]
   ```

2. **`LATTICE_DIAMOND_MAC_ADDRESS`**
   ```bash
   # Find MAC address from license email or file
   # Add as GitHub Secret
   # Name: LATTICE_DIAMOND_MAC_ADDRESS
   # Value: AB:00:00:00:00:00
   ```

### Workflow Jobs

**Job 1: build-firmware**
```yaml
- Runs on: ubuntu-latest
- Steps:
  1. Checkout repository
  2. Decode license from base64 secret
  3. Run: ./docker_build.sh release --force-clean
  4. Upload artifacts: sc64-firmware-*.bin, sc64-extra-*.zip
  5. On release: Upload to GitHub releases
```

**Job 2: build-deployer**
```yaml
- Matrix: Windows (32/64-bit), Linux, macOS
- For each platform:
  1. Checkout code
  2. Install Rust toolchain
  3. Build deployer: cargo build --release
  4. Package executable
  5. Upload platform-specific artifact
  6. On release: Upload to GitHub releases
```

**Job 3: publish-website**
```yaml
- Runs on: ubuntu-latest
- Triggers: Only on main branch
- Steps:
  1. Copy BOM from PCB project
  2. Deploy web/ directory to GitHub Pages
```

### Local CI Simulation

To simulate CI pipeline locally:

```bash
# 1. Encode your license for testing
base64 fw/project/lcmxo2/license.dat > /tmp/license_b64.txt

# 2. Decode it (simulating CI)
cat /tmp/license_b64.txt | base64 -d > /tmp/test_license.dat

# 3. Build with test license
cp /tmp/test_license.dat fw/project/lcmxo2/license.dat
MAC_ADDRESS="AB:00:00:00:00:00" ./docker_build.sh release --force-clean

# 4. Verify output
ls -lah sc64-*.zip sc64-*.bin
```

---

## Additional References

- **Lattice Diamond**: https://www.latticesemi.com/Software/diamondcad
- **Docker**: https://docs.docker.com/
- **Rust/Cargo**: https://doc.rust-lang.org/cargo/
- **ARM GCC Toolchain**: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm
---

## Verified Build Notes (Windows, 2026-08-03)

> This section records findings from a full successful build on Windows 11 with Docker Desktop 29.6.2.
> Use it to correct or supplement the sections above when merging.

### Windows: `docker_build.sh` cannot be called directly

On Windows, `wsl -- ./docker_build.sh` and `bash -c './docker_build.sh'` both fail:
```
Cannot connect to the Docker daemon at unix:///var/run/docker.sock
```
Those shells cannot reach the Docker Desktop daemon. Use `docker run` from PowerShell directly:

```powershell
$mac     = "6a:84:89:20:96:aa"   # HOSTID value from license.dat, with colons
$wd      = "$(Get-Location)"
$license = "$wd\fw\project\lcmxo2\license.dat"
$image   = "ghcr.io/polprzewodnikowy/sc64env:v1.10"

docker run --rm --mac-address $mac `
    -v "${license}:/flexlm/license.dat" `
    -v "${wd}:/workdir" `
    -e SC64_VERSION="dev" `
    --platform linux/x86_64 -w /workdir `
    $image ./build.sh release
```

Also needed once: `docker context use desktop-linux`

### License file format (modern)

The received `license.dat` did **not** have a `SERVER` line. The MAC address is embedded inside
each `FEATURE` line as `HOSTID=<hex>`:

```
FEATURE LSC_DIAMOND_A lattice 10.0 03-aug-2027 uncounted F088B32439E9 \
        VENDOR_STRING="Diamond Free" HOSTID=6a84892096aa
```

Extract the MAC in PowerShell:
```powershell
(Select-String -Path license.dat -Pattern 'HOSTID=([0-9a-f]+)').Matches[0].Groups[1].Value
# Returns e.g. 6a84892096aa  ->  use as  6a:84:89:20:96:aa  for MAC_ADDRESS
```

The license must be bound to a **physical** NIC (Ethernet or WiFi) — not a vEthernet or WSL virtual adapter.

### Errors encountered and fixes

**`Invalid (inconsistent) license key`** — MAC_ADDRESS does not match HOSTID in the license.
Request a new license for the correct physical NIC MAC.

**`/bin/bash^M: bad interpreter`** — CRLF line endings in shell scripts.
Fixed by `.gitattributes` `eol=lf` rules added in this branch. If it recurs:
```bash
sed -i 's/\r$//' build.sh docker_build.sh fw/project/lcmxo2/build.sh sw/bootloader/Makefile sw/cic/build.sh sw/controller/build.sh
```

### Verified build results

| Output | Size |
|--------|------|
| `sc64-firmware-networkfusion-dev.bin` | 189 KB |
| `sc64-extra-networkfusion-dev.zip` | 2.7 MB |

FPGA synthesis (LCMXO2-7000HC TQFP144 speed-6):
- Maximum frequency: **102.934 MHz** (100 MHz required) - 0 timing errors
- SLICEs: 3165/3432 (92%), Block RAMs: 26/26 (100%), LUT4s: 72%
- Design is near resource capacity; significant RTL additions may not fit

Docker image actual pull size: **~2.3 GB** (not ~5 GB as estimated above)

Component sizes from build output:

| Component | Size | Toolchain |
|-----------|------|-----------|
| Bootloader | 203 KiB ELF | mips64-elf-gcc (MIPS VR4300) |
| Controller (app) | 21.8 KiB ELF | arm-none-eabi-gcc (Cortex-M0+) |
| Controller (loader) | 3.4 KiB ELF | arm-none-eabi-gcc |
| Controller (primer) | 3.4 KiB ELF | arm-none-eabi-gcc |
| CIC | 1.6 KiB ELF | arm-none-eabi-gcc |
| FPGA bitstream | ~600 KiB .jed | Lattice Diamond 3.13 |
