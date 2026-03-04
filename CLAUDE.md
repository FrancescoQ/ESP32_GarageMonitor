# Garage Monitoring System - Claude Code Context

> **Project Status**: Phase 1 Complete — Phases 2-3 sensors integrated
> **Location**: Venice (Marghera), Italy
> **Last Updated**: March 4, 2026

---

## 📚 Additional Documentation

For detailed information, see the following resources:

### Project Documentation
- **Main Project Plan**: @garage_monitoring_project_plan.md - Complete development plan with all phases
- **Coding Standards**: @docs/coding_standards.md - **MANDATORY** code style and best practices (indentation, braces, naming, etc.)
- **Documentation Directory**: @docs/ - All detailed technical documentation (security, permissions, etc.)

> **Note**: The @docs/ reference means all `.md` files in that directory are automatically included in the project context.

---

## Development Environment

**Claude runs inside a Docker container (claudebox)**. This means:
- **No access to PlatformIO (`pio`)** — do NOT attempt to compile, upload, or run `pio` commands
- The user will compile locally and paste any errors back
- Focus on writing correct code; the user handles build/upload/test cycles
- **Do NOT commit or push** — the user commits from the host machine, not from inside the container

---

## Project Overview

IoT-based garage monitoring system with SMS remote control for monitoring door status, environmental conditions, and flood detection. Designed for deployment in a garage in the Venice area with remote SMS-based control.

### Core Hardware
- **Controller**: ESP32 DevKit
- **Cellular**: SIM7000G module (Iliad Domotica SIM plan)
- **Display**: LCD 16x2 with I2C interface
- **Sensors**:
  - Reed switch (door open/closed detection)
  - BME280 (temperature and humidity)
  - XKC-Y25 capacitive sensor (water/flood detection)

### Key Features
- SMS-based door status monitoring and control
- Temperature and humidity monitoring
- Flood detection with priority alerts
- Granular SMS command authorization system
- Low-power operation with deep sleep (Phase 5)
- Modular OOP architecture

---

## Development Phases

### ✅ Phase 0: Architecture & Foundation (Week 1) - COMPLETE
- [x] PlatformIO project configured
- [x] Libraries installed (TinyGSM, BME280, LiquidCrystal_I2C, ArduinoJson)
- [x] SystemController skeleton created
- [x] main.cpp basic structure
- [x] Architecture documented
- [x] LCD, I2C, UART, GPIO all tested and working
- [x] Pin assignments defined in Config.h

### ✅ Phase 1: Door Monitoring + SMS Core (Week 2-3) - COMPLETE
**Delivered**:
- DoorSensor class (reed switch + software debounce)
- DoorController class (relay sequences: STOP → pause → action)
- Door facade (combines DoorSensor + DoorController)
- ModemHandler (SIM7000G: power-on, SIM unlock, network, SMS send/receive/delete)
- MessageParser (command parsing + sender verification + granular permissions)
- DisplayController (LCD 16x2, 3-page layout with notifications)
- ButtonController (CLOSE/OPEN/STOP/FUNC physical buttons)
- Full SMS command loop: receive → parse → authorize → execute → respond → delete
- Security: ADMIN/CONTROL/MONITOR permission levels, phone number normalization

### ✅ Phase 2: Environmental Monitoring (Week 4) - PARTIAL
- [x] EnvironmentalSensor class (BME280 driver via I2C)
- [x] Temperature/humidity displayed on LCD environment page
- [x] STATUS SMS includes temperature/humidity readings
- [ ] ~~Periodic SMS reports~~ — intentionally skipped (no unsolicited SMS spam)
- [ ] Temperature/humidity threshold alerts (future)

### ✅ Phase 3: Water Detection (Week 5) - COMPLETE
- [x] WaterSensor class (XKC-Y25 + software debounce)
- [x] Immediate SMS alert on water detection
- [x] Water status displayed on LCD environment page
- [x] Inverted container design for sensor protection

### 🔲 Phase 4: Configuration & Web UI (Week 6)
- Credit monitoring (Iliad 400 service)
- WiFi setup mode (web UI for configuration)
- Diagnostics and logging
- NVS storage for authorized users
- Multilingual SMS support (IT/EN command aliases + localized responses)

### 🔲 Phase 5: Power Management (Week 7+)
- PowerManager class
- Deep sleep implementation (runtime-configurable via NVS from Phase 4 — can be disabled from web UI or SMS if issues arise in the field)
- Wake-up triggers (SMS via RI pin, door change, water detection, periodic)
- SMS polling → URC notification migration (AT+CNMI)
- Power consumption optimization

---

## Architecture Design

### Event-Driven Loop
The system uses an event-driven `loop()` with boolean flags (`m_alertSent`, `m_doorWasOpen`) for door-open tracking and alert suppression — not a formal state machine. A state machine with explicit transitions (IDLE, SLEEP, etc.) may be introduced in Phase 5 when deep sleep adds real state complexity.

### Class Structure (Implemented)
```
SystemController (main coordinator)
├── Door (facade)
│   ├── DoorSensor (reed switch + debounce)
│   └── DoorController (relay sequences)
├── WaterSensor (XKC-Y25 + debounce)
├── EnvironmentalSensor (BME280)
├── ModemHandler (SIM7000G: init, SMS, signal)
├── MessageParser (command parsing + authorization)
├── DisplayController (LCD 16x2, 3 pages, notifications)
├── ButtonController (CLOSE/OPEN/STOP/FUNC buttons)
├── ConfigManager (Phase 4)
└── PowerManager (Phase 5)
```

### Design Patterns
- **Facade**: Door wraps DoorSensor + DoorController behind a single interface
- **Observer**: Sensors notify controller of events via callbacks

---

## Security Model (Phase 1+)

### Permission System
```cpp
enum Permission {
    PERM_STATUS = 0x01,  // Can request status
    PERM_CLOSE  = 0x02,  // Can close door
    PERM_OPEN   = 0x04,  // Can open door
    PERM_CONFIG = 0x08   // Can change settings
};
```

### Permission Levels
- **ADMIN**: Full access (STATUS + CLOSE + OPEN + CONFIG)
- **CONTROL**: Can monitor and close (STATUS + CLOSE) - e.g., family member
- **MONITOR**: View only (STATUS) - e.g., neighbor

### Authorization Flow
1. Parse sender phone number from incoming SMS
2. Normalize number format (+39xxxxxxxxxx)
3. Lookup in authorized users list
4. Check command against user's permissions
5. Execute if authorized, reject if not

**Security Benefits**:
- SMS spoofing difficult (cellular network authentication)
- No internet dependency = no remote attack surface
- Allowlist model (explicit authorization)
- Granular permissions (least privilege)

---

## Hardware Configuration

### Pin Assignments (defined in Config.h)
```
I2C Bus (shared: LCD + BME280):
- SDA: GPIO 21
- SCL: GPIO 22

UART (SIM7000G):
- TX: GPIO 17
- RX: GPIO 16
- PWRKEY: GPIO 4
- DTR: GPIO 18 (sleep control, Phase 5)

Sensors:
- Door Reed Switch: GPIO 5 (INPUT_PULLUP)
- Water Sensor: GPIO 32 (INPUT)

Relay Outputs:
- CLOSE: GPIO 25
- STOP: GPIO 26
- OPEN: GPIO 27

Manual Buttons (all INPUT_PULLUP):
- CLOSE: GPIO 33
- OPEN: GPIO 13
- STOP: GPIO 14
- FUNC: GPIO 15
```

### I2C Device Addresses (confirmed)
- LCD 16x2: 0x27
- BME280: 0x76

### Power Requirements
- Supply: USB-C 5V 2A
- Smoothing: 2200µF capacitor for SIM7000G peak current
- Consumption: TBD (measure in Phase 5)

---

## Development Guidelines

### Code Style
- **Modular OOP**: Clean class separation with single responsibilities
- **Hardware abstraction**: Don't access hardware directly from main logic
- **State-driven**: Use state machine for system behavior
- **Security first**: Always verify SMS sender before command execution
- **Relay safety**: Relays must NEVER activate without explicit user action — see @docs/safety.md
- **Simplicity**: Don't over-engineer - solve real problems first

### Testing Strategy
- Test hardware components individually before integration
- Use I2C scanner before assuming device addresses
- Test SMS communication early (highest risk item)
- Implement sender verification from Phase 1
- Measure power consumption in Phase 5

### PlatformIO Configuration
```ini
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
monitor_filters = colorize, esp32_exception_decoder
```

### Key Libraries
- `TinyGSM ^0.11.7` - SIM7000G communication
- `Adafruit BME280 ^2.2.2` - Temperature/humidity sensor
- `LiquidCrystal_I2C ^1.1.4` - LCD display
- `ArduinoJson ^6.21.3` - JSON parsing (future use)

---

## Risk Mitigation

| Risk | Impact | Mitigation |
|------|--------|------------|
| SIM7000G communication issues | High | Test early in Phase 1, have fallback plan |
| Unauthorized SMS commands | High | Sender verification from Phase 1 |
| Poor cellular signal in garage | High | Test signal strength early, external antenna |
| Water sensor false positives | Medium | Inverted container, software debouncing |
| Power supply insufficient | Medium | Measure early, use adequate capacitance |
| Deep sleep not waking on SMS | Medium | Test wake sources independently |

---

## Success Criteria

### Phase 1 Complete
- ✓ Door open/close detection working
- ✓ SMS sent and received reliably
- ✓ Sender verification implemented
- ✓ Display shows current status
- ✓ CLOSE command executes

### Final System
- ✓ All sensors integrated and working
- ✓ SMS authorization secure and functional
- ✓ Deep sleep reduces power >80%
- ✓ System runs reliably for weeks
- ✓ Installed in garage and field-tested

---

## Quick Reference

**Serial Monitor**: 115200 baud
**Debug Level**: CORE_DEBUG_LEVEL=5
**Project Root**: `/Users/fquagliati/claude/garage_monitor`
**Main Plan**: `garage_monitoring_project_plan.md`

---

## Notes for Claude Code

- **Phase 1 is complete** — all core functionality (door, SMS, auth, display, buttons) is working
- **Next focus: Phase 4** (web UI, NVS configuration, credit monitoring)
- Phase 2 environmental monitoring is partial (BME280 works, threshold alerts still TODO)
- Security (SMS sender verification) is implemented and working
- Keep main.cpp minimal - all logic in SystemController and subsystems
- Use Serial.println() liberally for debugging
- Relay safety: see @docs/safety.md — relays must NEVER activate without explicit user action
- SMS functions live in ModemHandler (pragmatic choice; may extract to SMSHandler later)

---

**Author**: Francesco
**Project Start**: February 2026
**Target Deployment**: Garage in Marghera (Venice), Italy
