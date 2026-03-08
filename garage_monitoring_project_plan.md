# Garage Monitoring System - Development Plan

## Project Overview

IoT-based garage monitoring and control system for remote monitoring of door status, environmental conditions, and flood detection via SMS communication. Located in Marghera (Venice area), with SMS-based control from remote location.

**Core Requirements:**
- Remote door status monitoring and closure command
- Temperature/humidity monitoring
- Water detection for flood prevention
- SMS-based communication (Iliad Domotica plan)
- Low power consumption with deep sleep
- Modular, maintainable architecture

---

## Software Development Phases

### Phase 0: Architecture & Foundation (Week 1) — COMPLETE

**Objectives:**
- Design clean OOP architecture with proper class structure
- Establish hardware abstraction layers
- Set up development environment
- Define communication protocols

**Deliverables:**

#### Class Structure
```
├── main.cpp (minimal, just initialization and routing)
├── SystemController (main coordinator)
├── HardwareManager
│   ├── DoorSensor
│   ├── EnvironmentalSensor
│   ├── WaterSensor
│   └── DisplayController
├── CommunicationManager
│   ├── SMSHandler
│   ├── ATCommandInterface
│   └── MessageParser
├── PowerManager
│   ├── DeepSleepController
│   └── WakeupHandler
└── ConfigManager
    ├── PersistentStorage (EEPROM/SPIFFS)
    └── SetupMode (future WiFi config)
```

#### Key Design Patterns
- State Machine for system states (IDLE, MONITORING, ALERT, SLEEP, SETUP)
- Observer pattern for sensor events
- Factory pattern for sensor instantiation
- Singleton for communication manager

**Testing:**
- Basic ESP32 boot and LCD "Hello World"
- UART communication test
- GPIO test with LED

---

### Phase 1: Door Monitoring + SMS Core (Week 2-3) — COMPLETE

**Objectives:**
- Implement door sensor monitoring
- Establish SIM7000G communication
- Send SMS notifications
- Display status on integrated screen
- Implement "CLOSE" command reception

**Implementation Steps:**

1. **DoorSensor Class**
   - Reed switch GPIO with pullup
   - Debouncing logic (software or hardware)
   - State change detection
   - Event callbacks

2. **SMSHandler Class**
   - SIM7000G initialization sequence
   - AT command interface (async or blocking)
   - SMS send functionality
   - SMS receive and parsing
   - Extract sender phone number from SMS
   - Network registration checks
   - **SMS storage management**: Delete processed messages after reading (`AT+CMGD`)
   - Periodic purge of read messages to prevent storage full (SIM holds ~20-50 SMS)
   - Note: When SMS storage is full, new incoming messages are silently dropped by the network

3. **MessageParser Class + Security**
   - Parse incoming SMS commands
   - **Sender number verification** (authorized numbers only)
   - Hardcoded authorized numbers in Phase 1 (e.g., `const char* AUTHORIZED_NUMBERS[]`)
   - Reject commands from unknown senders
   - Handle international number format variations (+39, 0039, 39)
   - Support commands: STATUS, CLOSE
   - **Optional/Fun**: OPEN command (not strictly needed since you'll have the remote when physically present, but secure and fun to implement!)

4. **DisplayController Class**
   - Initialize 16x2 LCD via I2C
   - Status rendering (door open/closed)
   - Signal strength indicator
   - Last update timestamp
   - Multi-page display if needed (2 lines limited)

5. **Basic State Machine**
   - IDLE: door closed, no alerts
   - ALERT: door open >X minutes
   - COMMAND_PROCESSING: executing close command

**Testing Scenarios:**
- [x] Reed switch detects door open/close
- [x] SMS sent on door state change
- [x] SMS received and parsed correctly
- [x] **Sender number verification works** (reject unauthorized senders)
- [x] **Authorized numbers can send commands**
- [x] Display updates in real-time
- [x] "CLOSE" command triggers GPIO output
- [x] Handle network dropouts gracefully
- [x] International number format handled correctly
- [x] SMS deleted from storage after processing
- [x] System handles near-full SMS storage gracefully

**Hardware Setup:**
- ESP32 dev board
- SIM7000G module with antenna
- Reed switch with jumper cables
- Breadboard or flying wires
- USB-C power supply

---

### Phase 2: Environmental Monitoring (Week 4) — COMPLETE

**Objectives:**
- Add temperature and humidity sensing
- Periodic status reports via SMS
- Alert on abnormal readings

**Implementation Steps:**

1. **EnvironmentalSensor Class**
   - BME280/DHT22 driver integration
   - I2C/OneWire communication
   - Reading validation and filtering
   - Threshold configuration

2. **Periodic Reporting**
   - Timer-based readings (e.g., every 6 hours)
   - SMS report formatting
   - Historical data storage (optional)

3. **Alert Conditions**
   - Temperature thresholds (e.g., <0°C or >40°C)
   - Humidity thresholds (e.g., >90%)
   - Configurable via SMS commands

**Display Updates:**
- Show temperature and humidity on screen
- Add environmental icons
- Trend indicators (rising/falling)

**Testing Scenarios:**
- [x] Accurate temperature readings
- [x] Accurate humidity readings
- ~~Periodic SMS reports~~ — intentionally skipped (no unsolicited SMS spam)
- ~~Alerts trigger on threshold breach~~ — intentionally deferred
- [x] Display shows all environmental data

**Hardware Addition:**
- BME280 or DHT22 sensor
- Connect via I2C or GPIO (depending on sensor)

---

### Phase 3: Water Detection (Week 5) — COMPLETE

**Objectives:**
- Implement flood detection
- High-priority alert mechanism
- Critical SMS notifications

**Implementation Steps:**

1. **WaterSensor Class**
   - XKC-Y25 capacitive sensor interface
   - Continuous monitoring during wake periods
   - Immediate alert triggering
   - Self-test functionality

2. **Priority Alert System**
   - Immediate SMS on water detection
   - Multiple notification attempts
   - Visual/audible alarm (optional buzzer)
   - Cannot be easily dismissed

3. **Deep Integration**
   - Water detection as wake-up source
   - Highest priority in state machine
   - Override other operations

**Testing Scenarios:**
- [x] Water sensor detects immersion reliably
- [x] SMS sent within seconds of detection
- [x] False positive prevention
- [x] System recovers after water removal
- [x] Works with inverted container design

**Hardware Addition:**
- XKC-Y25 sensor module
- Test with inverted container prototype
- Cable extension test

---

### Phase 4: Configuration & Web UI (Week 6) — COMPLETE

**Objectives:**
- Implement credit monitoring
- Add WiFi setup mode
- Logging and diagnostics
- Remote configuration
- NVS-backed user and settings persistence
- Italian SMS aliases and responses

**Implementation — Delivered:**

1. **ConfigManager Class** (NVS persistence)
   - User storage: phone, permissions bitmask, name (seeded from Secrets.h on first boot)
   - System settings: door alert delay, SMS poll interval, deep sleep flag, unknown SMS forwarding, auto-reboot config
   - `begin()` loads NVS, seeds defaults if empty
   - CRUD operations: `addUser()`, `removeUser()`, `updateUserPermissions()`, `findUserByPhone()`
   - `getSettings()` / `setSettings()` for system configuration

2. **WebUIController Class** (WiFi AP + HTTP server)
   - WiFi AP SSID: "GarageSetup" (password in Secrets.h)
   - Setup mode entry: hold FUNC button during boot
   - Frontend: Preact + Tailwind CSS, served from LittleFS (`data/` directory)
   - API endpoints:

   | Method | Endpoint | Description |
   |--------|----------|-------------|
   | GET | `/api/users` | List all users (phone, name, permissions) |
   | POST | `/api/users` | Add user |
   | DELETE | `/api/users?index=N` | Remove user |
   | GET | `/api/settings` | Get all system settings |
   | POST | `/api/settings` | Update settings (partial update) |
   | GET | `/api/diagnostics` | Live sensor readings |
   | POST | `/api/reboot` | Reboot ESP32 |

3. **Credit Monitoring**
   - CREDIT / CREDITO SMS command (requires PERM_CONFIG)
   - Sends SMS with body "credito" to Iliad service number "400"
   - Response from 400 forwarded to admins via unknown SMS forwarding

4. **Italian SMS Support**
   - Command aliases: STATUS/STATO, CLOSE/CHIUDI, OPEN/APRI, CREDIT/CREDITO
   - All SMS responses in Italian (e.g., "Chiusura porta garage in corso.", "Permesso negato.")

5. **Auto-Reboot Scheduling**
   - Configurable via NVS/web UI: enable/disable, interval in days (1-30), target hour (0-23 or -1 for any)
   - Optional admin notification before reboot
   - NVS flag tracks reboot state for post-reboot notification

6. **Additional Features**
   - Unknown SMS forwarding to admins (configurable on/off)
   - Boot SMS purge: deletes all queued SMS on startup, notifies admins
   - FUNC button 5-second long-press reboot (works in any mode)

**Testing Scenarios:**
- [x] Credit balance inquiry sent correctly
- [x] Setup mode activated via FUNC button at boot
- [x] Web UI accessible and functional
- [x] Configuration persists after reboot (NVS)
- [x] Diagnostics endpoint returns live sensor data
- [x] Italian command aliases work
- [x] Auto-reboot triggers on schedule
- [x] Unknown SMS forwarded to admins

---

### Phase 5: Power Management (Week 7+)

**Objectives:**
- Implement deep sleep functionality
- Optimize power consumption
- Define wake-up triggers
- Extend system runtime

**Pre-existing from Phase 4:**
- NVS `deepSleepEnabled` flag already exists and is togglable via web UI
- DTR pin (GPIO 18) already defined in Config.h for SIM7000G sleep control

**Implementation Steps:**

1. **PowerManager Class**
   - Deep sleep configuration
   - Use existing `deepSleepEnabled` NVS flag (already configurable via web UI)
   - When disabled, system stays awake and polls normally (higher power, but more responsive)
   - Wake-up source management (RTC, GPIO, SIM7000G interrupt)
   - Power consumption profiling
   - Sleep schedule calculation

2. **Wake-up Triggers**
   - Periodic checks (e.g., every 30 minutes)
   - Incoming SMS (SIM7000G RI pin)
   - Door state change (reed switch)
   - Water detection (sensor interrupt)
   - Button press (manual check)

3. **Graceful Transitions**
   - Save state before sleep
   - Quick resume from sleep
   - Minimize wake time
   - Handle incomplete operations

**Testing Scenarios:**
- [ ] System enters deep sleep correctly
- [ ] Wakes up on SMS reception
- [ ] Wakes up on door change
- [ ] Wakes up on water detection
- [ ] Periodic wake-ups occur
- [ ] Measure actual power consumption

---

## Hardware Development Phases

### Phase A: Initial Breadboard Testing (Parallel with SW Phase 1)

**Objectives:**
- Verify all components work individually
- Test basic connectivity
- Prove core functionality

**Setup:**
- ESP32 dev board on breadboard or standalone
- SIM7000G connected via jumper cables
- Reed switch with pull-up resistor
- Antenna connected
- USB-C power supply

**Tests:**
- [x] ESP32 boots and LCD shows "Hello World"
- [x] SIM7000G initializes and registers on network
- [x] Send test SMS successfully
- [x] Receive SMS successfully
- [x] Reed switch detects state changes
- [x] GPIO output can trigger relay (simulated door close)

**Notes:**
- Can skip traditional breadboard if not needed
- Flying wires acceptable for initial tests
- Focus on proving communication works

---

### Phase B: Prototype Assembly (Parallel with SW Phase 2-3)

**Objectives:**
- Integrate all sensors
- Test with realistic cable lengths
- Validate power requirements
- Prototype enclosure concepts

**Setup:**
- Move components to a more permanent arrangement
- Use JST connectors for modularity where practical
- Test BME280 sensor with short cable
- Test water sensor with cable extension
- Measure power consumption under load

**Components to Test:**
- All sensors connected simultaneously
- I2C bus sharing (LCD 16x2 and BME280 on same bus)
- Cable lengths representative of final installation
- Power supply capacity (5V 2A)
- Capacitor smoothing (2200µF available)
- Display visibility from various angles

**Tests:**
- [ ] All sensors read correctly when connected together
- [ ] No I2C conflicts or communication issues
- [ ] Cable lengths don't affect sensor readings
- [ ] Power supply handles peak current
- [ ] No brownout resets during SIM7000G transmission

---

### Phase C: Enclosure Design (Week 5-6)

**Objectives:**
- Design main control box
- Design water sensor housing
- Plan cable routing
- Select mounting methods

**Main Control Box Requirements:**
- Weatherproof rating (IP54 minimum)
- Accommodate ESP32, SIM7000G, BME280, LCD 16x2
- Panel mounts for:
  - GX12-3 or GX16-4 aviation connector (sensors)
  - USB-C panel mount (power)
  - Antenna SMA bulkhead connector
  - LCD 16x2 display (front panel cutout or external mount)
- Ventilation if needed (BME280 requires air flow)
- Access for button (manual check)

**Water Sensor Housing:**
- Inverted container design (sensor stays dry)
- Weighted base for stability
- Cable exit with strain relief
- 3-pin Superseal connector at cable end

**Reed Switch Mounting:**
- Adhesive mounting on door
- Simple cable routing
- 2-pin JST or Superseal connector

**Design Tools:**
- Hand-drawn sketches initially
- Measure components precisely
- Consider using project box or custom 3D print
- Plan panel cutouts carefully

---

### Phase D: Cable Fabrication & Connectors (Week 6-7)

**Objectives:**
- Fabricate all cables with proper connectors
- Implement modular connection strategy
- Test connector reliability

**Cable Assemblies:**

1. **Main Control Box Panel Mounts:**
   - GX12-3 or GX16-4 panel mount socket (into enclosure)
   - Connects to internal ESP32 via JST or terminal block

2. **Sensor Cable Extensions:**
   - Superseal 3-pin connectors (male/female pairs)
   - Allow cable extensions if needed
   - One pin unused for 2-wire sensors (economical)

3. **Water Sensor Cable:**
   - Sensor → Superseal 3-pin male
   - Extension → Superseal female to male
   - Control box → Superseal female to GX/JST

4. **Reed Switch Cable:**
   - Switch → 2-pin JST or Superseal
   - Extension if needed
   - Control box → GX/JST

5. **Power Cable:**
   - USB-C panel mount on enclosure
   - Internal connection to ESP32 power input

**Testing:**
- [ ] All connectors mate properly
- [ ] Good electrical contact (continuity test)
- [ ] Strain relief adequate
- [ ] Can disconnect and reconnect multiple times
- [ ] No signal degradation through connectors

---

### Phase E: Final Assembly & Installation (Week 8+)

**Objectives:**
- Complete physical construction
- Install in garage
- Commission system
- Field testing

**Assembly Steps:**

1. **Control Box Assembly:**
   - Mount ESP32 in enclosure
   - Mount SIM7000G with proper spacing
   - Mount BME280 (ventilated position)
   - Install panel-mount connectors
   - Wire internal connections
   - Test before sealing

2. **Sensor Installation:**
   - Mount water sensor in strategic location (lowest point)
   - Install reed switch on door and magnet on frame
   - Route cables neatly
   - Secure cables to prevent damage

3. **Control Box Installation:**
   - Mount in protected location
   - Connect power supply
   - Connect sensor cables
   - Secure antenna for best signal

4. **System Commissioning:**
   - Power on and verify boot
   - Check sensor readings
   - Send test SMS
   - Receive test SMS
   - Test door open/close detection
   - Test water sensor with small amount of water
   - Verify deep sleep and wake-up

**Field Testing Checklist:**
- [ ] All sensors reading correctly in installation environment
- [ ] SMS communication reliable
- [ ] LCD display readable from expected viewing distance
- [ ] Door close command works
- [ ] Water detection works
- [ ] System survives power cycle
- [ ] System survives network dropout
- [ ] Acceptable power consumption

---

## Development Tools & Resources

**Hardware:**
- ESP32 dev board (standard, without integrated display)
- LCD 16x2 with I2C interface (already available)
- SIM7000G module + antenna
- BME280 or DHT22 sensor
- XKC-Y25 capacitive water sensor
- Reed switch (NO or NC)
- Relay module for door control
- Various connectors (JST, Superseal, GX)
- Perfboard or veroboard (if needed)
- Project enclosure
- USB-C power supply (5V 2A)
- 2200µF capacitor for smoothing

**Software:**
- Arduino IDE or PlatformIO
- ESP32 board support
- Libraries: TinyGSM (SIM7000), Adafruit BME280, LiquidCrystal_I2C (display), ArduinoJson (web API)
- Built-in: WebServer, WiFi, Preferences (NVS), LittleFS (static file serving)
- Serial monitor for debugging
- AT command reference for SIM7000G

**Documentation:**
- SIM7000G AT command manual
- ESP32 datasheet
- Sensor datasheets
- Connector pinouts

---

## Security Model

### SMS Command Authorization

**Default Configuration (Secrets.h, seeded to NVS on first boot):**
```cpp
// Permission flags
enum Permission {
    PERM_STATUS = 0x01,  // Can request status
    PERM_CLOSE  = 0x02,  // Can close door
    PERM_OPEN   = 0x04,  // Can open door
    PERM_CONFIG = 0x08   // Can change settings
};

// Permission presets
const uint8_t PERM_ADMIN   = PERM_STATUS | PERM_CLOSE | PERM_OPEN | PERM_CONFIG;
const uint8_t PERM_CONTROL = PERM_STATUS | PERM_CLOSE;
const uint8_t PERM_MONITOR = PERM_STATUS;

// Authorized users with granular permissions
struct AuthorizedUser {
    const char* number;
    uint8_t permissions;
    const char* name;  // Optional, for logging
};

const AuthorizedUser AUTHORIZED_USERS[] = {
    {"+39xxxxxxxxxx", PERM_ADMIN,   "Francesco"},      // Full access
    {"+39yyyyyyyyyy", PERM_CONTROL, "Family Member"},  // Status + Close only
    {"+39zzzzzzzzzz", PERM_MONITOR, "Neighbor"},       // Status only
    {nullptr, 0, nullptr}  // Terminator
};
```

**Permission Levels:**
- **ADMIN**: Full control (STATUS, CLOSE, OPEN, CONFIG)
- **CONTROL**: Can monitor and close door (STATUS, CLOSE) - e.g., family member
- **MONITOR**: View only (STATUS) - e.g., neighbor checking while you're away

**Security Checks:**
1. **Sender Verification**: Every incoming SMS is checked against authorized users
2. **Permission Check**: Command is checked against user's permission flags
3. **Granular Access**: Different users can have different capabilities
4. **Rejection**: Unknown senders optionally forwarded to admins then ignored; authorized users without permission get "Permesso negato."

**Number Format Handling:**
- Support multiple formats: `+39xxxxxxxxxx`, `0039xxxxxxxxxx`, `39xxxxxxxxxx`
- Normalize to consistent format before comparison
- Case-insensitive command parsing (CLOSE = close = Close)

**Supported Commands:**
- `STATUS` / `STATO` - Returns door state, temperature, humidity, water status, signal, uptime
- `CLOSE` / `CHIUDI` - Triggers door close relay sequence (STOP → pause → CLOSE)
- `OPEN` / `APRI` - Triggers door open relay sequence (STOP → pause → OPEN)
- `CREDIT` / `CREDITO` - Sends credit inquiry to Iliad 400 service (ADMIN only)

**Security Benefits:**
- SMS spoofing is difficult (cellular network handles authentication)
- No internet dependency = no remote attack surface
- Simple allowlist model (easier to verify than blocklist)
- Works even if WiFi/internet is down

**Phase 4 Enhancements (Done):**
- ✅ Authorized users and permissions stored in NVS (via ConfigManager)
- ✅ Add/remove users via WiFi setup mode web UI
- ✅ Assign/revoke permissions via web UI
- ✅ Unknown SMS forwarding to admins (configurable)
- Future: Permission inheritance/roles system
- Future: Optional secret PIN for extra security (e.g., `CLOSE:1234`)

---

## Risk Mitigation

| Risk | Impact | Mitigation |
|------|--------|------------|
| SIM7000G communication issues | High | Start with Phase 1, test extensively, have fallback plan |
| Unauthorized command execution | High | **Sender number verification on every SMS command** |
| SMS spoofing attempts | Medium | Rely on cellular network authentication, consider optional PIN |
| Water sensor false positives | Medium | Inverted container design, software debouncing, test thoroughly |
| Power supply insufficient | Medium | Measure consumption early, use adequate capacitance |
| Poor cellular signal in garage | High | Test signal strength early, external antenna if needed |
| Deep sleep not waking on SMS | Medium | Test wake sources independently, hardware interrupt design |
| I2C issues over long cables | Low | Keep BME280 in main enclosure, test if external |
| Connector failure in field | Low | Use quality connectors, strain relief, test durability |

---

## Success Criteria

**Phase 1 Complete:** ✅
- ✓ System detects door open/close
- ✓ SMS sent and received reliably
- ✓ Display shows current status
- ✓ Door close command executed

**Phase 3 Complete:** ✅
- ✓ All sensors integrated and working
- ✓ Water detection triggers immediate alert
- ✓ Environmental data monitored
- ✓ Multiple SMS commands supported

**Phase 4 Complete:** ✅
- ✓ Configuration persists via NVS
- ✓ Web UI accessible for setup (WiFi AP "GarageSetup")
- ✓ Credit monitoring functional (CREDIT/CREDITO command)
- ✓ Italian SMS aliases and responses
- ✓ Auto-reboot scheduling
- ✓ Unknown SMS forwarding to admins
- ✓ Boot SMS purge for safety

**Phase 5 Complete:**
- Deep sleep reduces power consumption >80%
- System runs reliably for weeks
- Easy to maintain and update

**Final Installation:**
- System installed in garage
- All features tested in real environment
- User can monitor and control remotely
- System is weatherproof and reliable

---

## Timeline

- **Weeks 1-3 (Feb 2026):** Software Phases 0-1, Hardware Phase A — ✅ Complete
- **Weeks 4-5 (Mar 2026):** Software Phases 2-3, Hardware Phase B — ✅ Complete
- **Week 6 (Mar 2026):** Software Phase 4 — ✅ Complete
- **Week 7+:** Software Phase 5 (Power management) — Pending
- **TBD:** Hardware Phases C-E (Enclosure, cables, installation)

**Total Estimated Time:** 8-10 weeks for complete system

---

## Notes

- Development phases can overlap
- Testing is critical at each phase
- Keep modularity as top priority
- Document as you go
- Iterate based on real-world testing
- Don't over-engineer - solve real problems first

---

**Version:** 1.1
**Last Updated:** March 2026
**Author:** Francesco
**Project:** Venice Garage Monitoring System
