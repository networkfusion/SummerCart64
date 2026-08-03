# Firmware Build — Verified Working Reference

**Verified**: 2026-08-03 — full release build completed successfully on Windows with Docker Desktop  
**Repository**: networkfusion/SummerCart64  
**Branch**: docs/ai-agent-analysis  
**Audience**: Developers and AI agents building SummerCart64 firmware

---

## Result Summary (2026-08-03)

| Output | Size |
|--------|------|
| `sc64-firmware-networkfusion-dev.bin` | 189 KB |
| `sc64-extra-networkfusion-dev.zip` | 2.7 MB |

**FPGA**: LCMXO2-7000HC — 102.934 MHz (100 MHz required), 0 timing errors  
**Resources**: SLICEs 92%, Block RAMs 100%, LUT4s 72% — design is near capacity  
**Version embedded**: branch `docs/ai-agent-analysis`, tag `v2.20.2-28-g8e28c280`

---

## Executive Summary

Successfully building SummerCart64 firmware requires:
1. **Docker** (for reproducible builds across platforms)
2. **Lattice Diamond License** (1-year free personal license required for FPGA compilation)
3. **License File** placed at `fw/project/lcmxo2/license.dat`
4. **MAC Address** linked to the license (passed as environment variable)

This document provides evidence-based guidance based on direct analysis of:
- `build.sh` - Main build orchestration
- `docker_build.sh` - Docker wrapper script
- `.github/workflows/build.yml` - CI/CD pipeline configuration
- `docs/05_fw_and_sw_info.md` - Official build documentation
- `docs/06_build_guide.md` - Hardware/software setup guide

---

## Build Architecture Overview

### Build Flow Chart

```
User runs: ./docker_build.sh release --force-clean
    ↓
[docker_build.sh]
    - Mounts license file to: /flexlm/license.dat
    - Sets MAC_ADDRESS env var from input or default
    - Pulls docker image (if not cached)
    - Creates container with user ID preservation
    ↓
[Docker Container - sc64env:v1.10]
    - Contains: Lattice Diamond, ARM GCC, Rust, Python
    ↓
[./build.sh release]
    1. Extract git info (branch, tag, SHA, commit message)
    2. build_bootloader()
        - Compile sw/bootloader → binary included in firmware
    3. build_controller()
        - Compile sw/controller (includes CIC emulation)
        - Binary included in firmware
    4. build_fpga()
        - Invoke fw/project/lcmxo2/build.sh
        - Uses Lattice Diamond with license from /flexlm/license.dat
        - Requires correct MAC_ADDRESS
        - Output: FPGA bitstream
    5. build_update()
        - Package all binaries into sc64-firmware-{version}.bin
    6. Create sc64-extra-{version}.zip with all files
    ↓
Output: 
    - sc64-firmware-{version}.bin (main firmware)
    - sc64-extra-{version}.zip (release package)
```

## Windows Build Command (Copy-Paste Ready)

```powershell
# Verify Docker Desktop is running
docker context use desktop-linux
docker ps

$mac     = "AA:BB:CC:DD:EE:FF"   # replace with HOSTID from your license.dat
$license = "$(Get-Location)\fw\project\lcmxo2\license.dat"
$wd      = "$(Get-Location)"
$image   = "ghcr.io/polprzewodnikowy/sc64env:v1.10"

docker run --rm --mac-address $mac `
    -v "${license}:/flexlm/license.dat" `
    -v "${wd}:/workdir" `
    -e SC64_VERSION="dev" `
    --platform linux/x86_64 -w /workdir `
    $image ./build.sh release
```

> **Do NOT use**: `wsl -- ./docker_build.sh` or `bash -c './docker_build.sh'` — both fail on Windows.

## License Requirements (Verified)

### What the license file looks like

Modern Lattice Diamond free licenses **do not have a `SERVER` line**. The MAC address is embedded
inside each `FEATURE` line as `HOSTID=<hex>`:

```
FEATURE LSC_DIAMOND_A lattice 10.0 03-aug-2027 uncounted F088B32439E9 \
        VENDOR_STRING="Diamond Free" HOSTID=6a84892096aa
```

- `HOSTID=6a84892096aa` → MAC address without colons
- `MAC_ADDRESS` env var → same value **with colons**: `6a:84:89:20:96:aa`
- `LSC_DIAMOND_A` → this is the feature required; confirms it is the Diamond Free license
- Expiry in the `FEATURE` line (e.g. `03-aug-2027`)

### Extracting MAC from license (PowerShell)

```powershell
# Returns e.g. 6a84892096aa
(Select-String -Path fw\project\lcmxo2\license.dat -Pattern 'HOSTID=([0-9a-f]+)').Matches[0].Groups[1].Value
```

### Requesting a correct license

1. Find your physical NIC MAC: `Get-NetAdapter | Where Status -eq Up | Select Name, MacAddress`
2. Use a **physical** adapter (Ethernet or WiFi) — NOT vEthernet, WSL, or loopback
3. Visit https://www.latticesemi.com/Support/Licensing
4. Select **Free Personal License**, device family **MachXO2** (exactly — not MachXO3, not ECP5)
5. Skip Crosslink IP and other optional add-ons
6. Enter your physical NIC MAC address
7. Receive `license.dat` by email, copy to `fw/project/lcmxo2/license.dat`

---

## Troubleshooting Quick Reference

| Symptom | Cause | Fix |
|---------|-------|-----|
| `Cannot connect to Docker daemon` | Running from bash/WSL on Windows | Use PowerShell `docker run` directly; `docker context use desktop-linux` |
| `/bin/bash^M: bad interpreter` | CRLF line endings in shell scripts | `bash -c "sed -i 's/\r$//' build.sh docker_build.sh ..."` |
| `Invalid (inconsistent) license key` | License bound to different MAC | Request new license for your physical NIC MAC |
| `License for device ... not found` | Wrong device family | Request license specifying **MachXO2** |
| `MAC address mismatch` | `MAC_ADDRESS` env var wrong format or wrong value | Must be colon-separated (`AA:BB:CC:DD:EE:FF`), must match `HOSTID=` in license |
| `Image not found` | Docker image not pulled | First run pulls ~2.3 GB; wait or `docker pull ghcr.io/polprzewodnikowy/sc64env:v1.10` |
| `Timing errors: N Score: N` | FPGA timing violation | Design is at 92% utilisation; significant RTL additions may need optimisation |

---

## License Acquisition Workflow

### Step-by-Step Evidence-Based Process

#### 1. Request License

**Official Source**: https://www.latticesemi.com/Support/Licensing

**Process**:
- Visit Lattice website
- Select "Personal License" (free, 1-year validity)
- Enter email, name, company (optional)
- Select device family: **MachXO2** (CRITICAL)
- Specify development activity
- Submit request

**Evidence**: Project uses MachXO2 FPGA (confirmed in hw/pcb/sc64v2.kicad_sch via device part numbers LCMXO2-4000HC)

#### 2. Receive License

**Email contains**:
- `license.dat` file (text or binary format)
- MAC address: Format like `AB:00:00:00:00:00` (colons, not hyphens)
- License duration (1 year from issue)
- License ID and key information

**Important**: MAC address MUST match the license file. This is how Lattice Diamond verifies license validity.

#### 3. Install License

```bash
# Step A: Place license file in repository
cp ~/Downloads/license.dat fw/project/lcmxo2/license.dat

# Step B: Extract MAC address from license
# Option 1: From email
# Option 2: From file (if contains MAC info):
grep -i "MAC\|HOST" fw/project/lcmxo2/license.dat

# Step C: Build with MAC address
export MAC_ADDRESS="AB:00:00:00:00:00"  # Use YOUR MAC address
./docker_build.sh fpga
```

---

## All Build Targets

**Evidence source**: `build.sh` function definitions

| Target | Command (Linux/macOS) | Time | Requires License | Verified Output |
|--------|----------------------|------|------------------|-----------------|
| **bootloader** | `./docker_build.sh bootloader` | ~1 min | No | 203 KiB ELF (MIPS N64) |
| **controller** | `./docker_build.sh controller` | ~2 min | No | 3 profiles: primer 3.4 KiB, loader 3.4 KiB, app 21.8 KiB |
| **cic** | `./docker_build.sh cic` | ~1 min | No | 1.6 KiB ELF |
| **fpga** | `./docker_build.sh fpga` | ~26 min | **Yes** | FPGA bitstream, 102.934 MHz |
| **update** | `./docker_build.sh update` | ~1 min | No | `sc64-firmware-{version}.bin` |
| **release** | `./docker_build.sh release` | ~5 min after FPGA | **Yes** | `.bin` + `.zip` |

### Typical Development Workflow

**For modifying bootloader or controller** (no license needed):
```bash
# Edit sw/bootloader/src/* or sw/controller/src/*
./docker_build.sh bootloader
./docker_build.sh controller
# Firmware builds successfully (FPGA part is pre-built)
```

**For modifying FPGA logic** (license required):
```bash
# Edit fw/rtl/*.sv
MAC_ADDRESS="AB:00:00:00:00:00" ./docker_build.sh fpga
# Lattice Diamond compiles new FPGA bitstream
```

**For release build** (license required):
```bash
SC64_VERSION="v3.0" MAC_ADDRESS="AB:00:00:00:00:00" ./docker_build.sh release --force-clean
# Produces: sc64-firmware-v3.0.bin and sc64-extra-v3.0.zip
```

---

## CI/CD Integration (GitHub Actions)

**Evidence from .github/workflows/build.yml** (lines 1-50):

```yaml
- name: Retrieve the Lattice Diamond license from secrets and decode it
  run: echo $LICENSE | base64 --decode > fw/project/lcmxo2/license.dat
  env:
    LICENSE: ${{ secrets.LATTICE_DIAMOND_LICENSE_BASE64 }}

- name: Build firmware
  run: ./docker_build.sh release --force-clean
  env:
    MAC_ADDRESS: ${{ secrets.LATTICE_DIAMOND_MAC_ADDRESS }}
    SC64_VERSION: ${{ steps.version.outputs.replaced }}
```

**How it works**:
1. License is stored as base64-encoded string in GitHub Secrets
2. At build time, GitHub Actions decodes it to `fw/project/lcmxo2/license.dat`
3. MAC address is passed from GitHub Secrets as environment variable
4. Docker build uses both to compile FPGA

**Setup for repository maintainers**:
```bash
# 1. Encode license
cat fw/project/lcmxo2/license.dat | base64 > license_base64.txt

# 2. Add to GitHub Secrets
# Go to: Settings → Secrets and variables → Actions
# Create secret: LATTICE_DIAMOND_LICENSE_BASE64
# Value: [paste base64 content]

# 3. Create MAC address secret
# Create secret: LATTICE_DIAMOND_MAC_ADDRESS
# Value: AB:00:00:00:00:00 (from license email)
```

---

## Troubleshooting Matrix

### License Not Found

| Error Message | Root Cause | Evidence | Solution |
|---------------|-----------|----------|----------|
| `FLEXlm: License not found` | File missing or path wrong | License expected at `fw/project/lcmxo2/license.dat` (docker_build.sh line 10) | `cp license.dat fw/project/lcmxo2/` |
| `License for product "MachXO2" not found` | License doesn't include MachXO2 | build.sh requires device family match | Request new license, select MachXO2 |
| `MAC address mismatch` | MAC format wrong or address doesn't match license | docker_build.sh passes `--mac-address` flag to docker | Use format `AB:00:00:00:00:00` (colons, not hyphens) |
| `Permission denied` on license.dat | File permissions issue | Docker runs with user:group context | `chmod 644 fw/project/lcmxo2/license.dat` |

### Docker Build Issues

| Error | Cause | Solution |
|-------|-------|----------|
| `Cannot connect to Docker daemon` | Docker not running | Windows: Start Docker Desktop; Linux: `sudo systemctl start docker` |
| `Image not found` | Image not pulled yet | `docker pull ghcr.io/polprzewodnikowy/sc64env:v1.10` |
| `Cannot mount volume` | File doesn't exist | Ensure `fw/project/lcmxo2/license.dat` exists before build |

---

## Performance Expectations

### Build Time Breakdown

**First run** (includes 2.3 GB Docker image pull): ~35 minutes total  
**Subsequent runs** (image cached): FPGA ~26 min + everything else ~5 min = ~31 min  
**Incremental** (FPGA already built, only bootloader/controller changed): ~5 min

