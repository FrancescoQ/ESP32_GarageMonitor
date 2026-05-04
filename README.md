# Garage Monitor

IoT garage monitoring system with SMS remote control, built on ESP32 + SIM7000G.

Monitors door status, temperature/humidity, and flood detection. Controlled via SMS commands with granular per-user permissions.

## Quick Start

### Prerequisites

- [PlatformIO CLI](https://platformio.org/install/cli) or PlatformIO IDE extension for VS Code
- ESP32 connected via USB

### Build & Upload

| Task | CLI Command |
|------|-------------|
| Compile firmware | `pio run` |
| Compile + upload firmware | `pio run -t upload` |
| Build filesystem (LittleFS) | `pio run -t buildfs` |
| Build + upload filesystem via USB | `pio run -t uploadfs` |
| Serial monitor | `pio device monitor` |
| Clean build | `pio run -t clean` |

### Web UI

The web UI (Settings, Diagnostics) is a Preact app served from LittleFS.

```bash
cd web_ui_src
npm install        # first time only
npm run build      # builds to data/ directory
```

After building, upload the filesystem to the ESP32:

```bash
pio run -t uploadfs
```

## OTA Updates

The system supports over-the-air updates via [ElegantOTA](https://github.com/ayushsharma82/ElegantOTA).

### How to access

1. Hold the **FUNC** button while powering on the ESP32 to enter **Setup Mode**
2. Connect to the `GarageSetup` WiFi network
3. Browse to `http://192.168.4.1/update`

### Firmware update (OTA)

1. Compile: `pio run`
2. The firmware binary is at: `.pio/build/esp32dev/firmware.bin`
3. On the `/update` page, select **Firmware**, pick the `.bin` file, and upload

### Filesystem update (OTA)

1. Build the web UI: `cd web_ui_src && npm run build`
2. Build the filesystem image: `pio run -t buildfs`
3. The image is at: `.pio/build/esp32dev/littlefs.bin`
4. On the `/update` page, select **Filesystem**, pick the `.bin` file, and upload

### When do I need to update what?

| What changed | Rebuild | Upload |
|---|---|---|
| C++ source code (`src/`, `include/`) | `pio run` | Firmware |
| Web UI (`web_ui_src/`) | `npm run build` then `pio run -t buildfs` | Filesystem |
| Both | Both of the above | Both (firmware first, then filesystem) |

## Setup Mode

Hold the **FUNC** button during boot to enter setup mode. This starts a WiFi AP (`GarageSetup`) with a web UI for:

- Managing authorized users and permissions
- Configuring system settings (alert thresholds, reboot schedule, etc.)
- Viewing live sensor diagnostics
- OTA firmware/filesystem updates

## SMS Commands

| Command | Alias (IT) | Permission | Description |
|---------|-----------|------------|-------------|
| STATUS | STATO | MONITOR | Door state, sensors, signal, uptime, version |
| CLOSE | CHIUDI | CONTROL | Close garage door |
| OPEN | APRI | OPEN | Open garage door |
| CREDIT | CREDITO | CONFIG | Check SIM credit balance |
| HELP | AIUTO | MONITOR | List available commands |
| REBOOT | RIAVVIO | CONFIG | Restart the system |

### Permission Levels

- **ADMIN** — Full access (all commands + config)
- **CONTROL** — STATUS + CLOSE
- **MONITOR** — STATUS only
