# Garage Monitoring System - Claude Code Context

> **Project Status**: Phase 0 - Architecture & Foundation
> **Location**: Venice (Marghera), Italy
> **Last Updated**: February 12, 2026

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
- **Controller**: ESP32-C3 DevKit M-1
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
- Low-power operation with deep sleep (Phase 4)
- Modular OOP architecture

---

## Development Phases

### ✅ Phase 0: Architecture & Foundation (Week 1) - IN PROGRESS
**Status**: Bare minimum setup complete, ready to start implementation

**Completed**:
- [x] PlatformIO project configured
- [x] Libraries installed (TinyGSM, BME280, LiquidCrystal_I2C, ArduinoJson)
- [x] SystemController skeleton created
- [x] main.cpp basic structure
- [x] Architecture documented

**Current Tasks**:
- [ ] LCD "Hello World" test
- [ ] I2C scanner implementation
- [ ] GPIO LED test
- [ ] UART communication test
- [ ] Pin assignments documentation

### 🔲 Phase 1: Door Monitoring + SMS Core (Week 2-3)
**Key deliverables**:
- DoorSensor class (reed switch monitoring with debouncing)
- SMSHandler class (SIM7000G AT commands, send/receive, **SMS storage management**)
- MessageParser class with **sender verification and permissions**
- DisplayController class (LCD status display)
- Basic state machine (IDLE, ALERT, COMMAND_PROCESSING)

**Security Model**:
- Granular permission system (ADMIN, CONTROL, MONITOR)
- SMS sender number verification (allowlist only)
- Support for STATUS and CLOSE commands
- Optional OPEN command (secure and fun)

### 🔲 Phase 2: Environmental Monitoring (Week 4)
- EnvironmentalSensor class (BME280)
- Periodic SMS reports (e.g., every 6 hours)
- Temperature/humidity alerts on threshold breach

### 🔲 Phase 3: Water Detection (Week 5)
- WaterSensor class (XKC-Y25)
- Priority alert system (immediate SMS)
- Inverted container design for sensor protection

### 🔲 Phase 4: Power Management (Week 6)
- PowerManager class
- Deep sleep implementation
- Wake-up triggers (SMS, door change, water detection, periodic)
- Power consumption optimization

### 🔲 Phase 5: Advanced Features (Week 7+)
- Credit monitoring (Iliad 400 service)
- WiFi setup mode (web UI for configuration)
- Diagnostics and logging
- NVS storage for authorized users

---

## Architecture Design

### State Machine
```cpp
enum class SystemState {
    IDLE,                  // Door closed, no alerts
    MONITORING,            // Active monitoring
    ALERT,                 // Door open >X minutes
    SLEEP,                 // Deep sleep mode
    SETUP,                 // WiFi configuration mode
    COMMAND_PROCESSING     // Executing SMS command
};
```

### Class Structure
```
SystemController (main coordinator)
├── HardwareManager
│   ├── DoorSensor
│   ├── EnvironmentalSensor
│   ├── WaterSensor
│   └── DisplayController
├── CommunicationManager
│   ├── SMSHandler
│   ├── ATCommandInterface
│   └── MessageParser
├── PowerManager (Phase 4)
│   ├── DeepSleepController
│   └── WakeupHandler
└── ConfigManager (Phase 5)
    ├── PersistentStorage
    └── SetupMode
```

### Design Patterns
- **State Machine**: For system state management
- **Observer**: Sensors notify controller of events
- **Factory**: Sensor instantiation
- **Singleton**: CommunicationManager

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

### Pin Assignments (To be defined in Phase 1)
```
I2C Bus (shared):
- SDA: GPIO ? (LCD + BME280)
- SCL: GPIO ? (LCD + BME280)

UART (SIM7000G):
- TX: GPIO ?
- RX: GPIO ?
- PWRKEY: GPIO ?
- RI: GPIO ? (SMS wake-up, Phase 4)

GPIO:
- Door Reed Switch: GPIO ? (INPUT_PULLUP)
- Water Sensor: GPIO ? (INPUT)
- Door Close Relay: GPIO ? (OUTPUT)
- Door Open Relay: GPIO ? (OUTPUT, optional)
```

### I2C Device Addresses
- LCD 16x2: 0x27 or 0x3F (verify with scanner)
- BME280: 0x76 or 0x77 (default 0x76)

### Power Requirements
- Supply: USB-C 5V 2A
- Smoothing: 2200µF capacitor for SIM7000G peak current
- Consumption: TBD (measure in Phase 4)

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
- Measure power consumption in Phase 4

### PlatformIO Configuration
```ini
platform = espressif32
board = esp32-c3-devkitm-1
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

- This is **Phase 0/Phase 1** - focus on foundation and basic functionality
- Start with minimal implementations, expand in later phases
- Security (SMS sender verification) is CRITICAL from Phase 1
- Test SIM7000G communication early - it's the highest risk item
- Keep main.cpp minimal - all logic in SystemController and subsystems
- Use Serial.println() liberally for debugging
- Document pin assignments as they are defined
- OPEN command is optional but secure and fun to implement

---

**Author**: Francesco
**Project Start**: February 2026
**Target Deployment**: Garage in Marghera (Venice), Italy
