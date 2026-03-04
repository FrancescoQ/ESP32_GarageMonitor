# Phase 1 Sub-Phase Breakdown

## Context

Phase 1 ("Door Monitoring + SMS Core") is the largest phase in the project — it delivers the entire communication backbone, security model, and command processing pipeline. Rather than tackling it all at once, we split it into 5 incremental sub-phases, each independently testable on real hardware.

### What Already Works (Phase 0)
- **ModemHandler**: SIM7000G power-on, SIM PIN unlock, network registration, SMS send, signal monitoring (~273 lines, solid)
- **SystemController**: basic door monitoring loop (reed switch on GPIO 5, relay on GPIO 25) — works but logic is inline/procedural
- **Config.h**: all pin assignments and timing constants defined
- **main.cpp**: minimal, delegates to SystemController

### Key Finding
TinyGSM has **no SMS receive support** — only `sendSMS`. All receive/read/delete must be implemented via raw AT commands (`AT+CMGL`, `AT+CMGR`, `AT+CMGD`) through ModemHandler's existing `sendCommand()` method. This is the highest-risk item.

---

## Dependency Graph

```
1A (SMS Receive) ──────────┐
                           │
1B (DoorSensor) ───────────┼──> 1E (Integration + State Machine)
                           │
1C (MessageParser + Auth) ─┤
                           │
1D (DisplayController) ────┘
```

- **1A first** (highest risk, blocks everything downstream)
- **1B, 1C, 1D** can be done in any order (independent of each other)
- **1E last** (wires everything together)

---

## Sub-Phase 1A: SMS Receive via AT Commands

**Goal**: Add SMS receive, read, and delete to ModemHandler. Prove the system can poll for incoming SMS and read them reliably.

**Why first**: Highest risk — no library support, raw AT command parsing needed. Everything downstream depends on this.

**Files to modify**:
- `include/ModemHandler.h` — add ReceivedSMS struct, new public methods
- `src/ModemHandler.cpp` — implement SMS receive/read/delete via AT commands
- `src/SystemController.cpp` — add a simple polling loop that prints received SMS to Serial

**New functionality on ModemHandler**:
```cpp
struct ReceivedSMS {
  int index;
  String sender;
  String message;
  String timestamp;
};

int checkForSMS();                          // AT+CMGL="REC UNREAD", returns count
bool readSMS(int index, ReceivedSMS& sms);  // AT+CMGR=<index> (marks as read)
bool deleteSMS(int index);                  // AT+CMGD=<index>
bool deleteReadSMS();                       // AT+CMGD=1,1 (batch delete read)
bool deleteAllSMS();                        // AT+CMGD=1,4 (emergency purge)
```

**AT response formats to parse** (text mode, already enabled):
- `AT+CMGL="ALL"` → `+CMGL: <idx>,"status","number","","timestamp"\n<body>\nOK`
- `AT+CMGR=<idx>` → `+CMGR: "status","number","","timestamp"\n<body>\nOK`

**Hardware test**: Send SMS from phone → Serial monitor shows sender + message → message deleted from SIM.

**Commit boundary**: ModemHandler can receive/read/delete SMS. SystemController polls every 5s and prints to Serial. No parsing yet.

---

## Sub-Phase 1B: DoorSensor Class

**Goal**: Extract reed switch logic from SystemController into a proper class with debouncing and event tracking.

**Why**: Pure refactoring of working code — low risk, cleans up SystemController for the final integration.

**Files to create**:
- `include/DoorSensor.h`
- `src/DoorSensor.cpp`

**Files to modify**:
- `include/SystemController.h` — add `DoorSensor m_doorSensor` member
- `src/SystemController.cpp` — replace inline reed switch code (lines 44-69) with DoorSensor calls

**DoorSensor class**:
- `begin(int pin)` / `loop()` lifecycle
- Software debouncing using `DOOR_DEBOUNCE_MS` (50ms, already in Config.h)
- `isDoorOpen()`, `hasStateChanged()`, `getOpenDurationMs()`
- Callback via `onStateChange(DoorStateCallback)`
- Tracks when door opened for alert timing (`DOOR_ALERT_DELAY_MS`)

**Hardware test**: Behavior identical to current system — open/close door, verify Serial output and relay activation unchanged.

**Commit boundary**: DoorSensor class works. SystemController uses it. Behavior unchanged.

---

## Sub-Phase 1C: MessageParser + Permission System

**Goal**: Build command parsing and authorization. This is the security backbone.

**Files to create**:
- `include/MessageParser.h` — ParseResult, SMSCommand enum, permission flags, AuthorizedUser struct
- `src/MessageParser.cpp` — parsing, normalization, permission checking
- `include/AuthConfig.h` — authorized users list (separate from hardware Config.h)

**Key components**:
- **Permission flags**: `PERM_STATUS=0x01`, `PERM_CLOSE=0x02`, `PERM_OPEN=0x04`, `PERM_CONFIG=0x08`
- **Permission presets**: ADMIN (all), CONTROL (status+close), MONITOR (status only)
- **Phone number normalization**: handle `+39`, `0039`, `39` formats, strip spaces/dashes (implementation already in `docs/security.md` lines 99-114)
- **Command parsing**: case-insensitive, trim whitespace, exact match for STATUS/CLOSE/OPEN
- **ParseResult struct**: command, isAuthorized, hasPermission, userName

**Authorization flow** (matches `docs/security.md` flowchart):
1. Normalize sender number
2. Lookup in authorized users → not found = silently ignore
3. Parse command → unknown = reply "Unknown command"
4. Check permission → denied = reply "Permission denied"
5. Authorized + permitted = execute

**Testing**: Inline test function in `setup()` with hardcoded test cases (authorized admin, unauthorized sender, monitor trying CLOSE, number format variations).

**Commit boundary**: MessageParser works as standalone logic. Not yet wired into SMS receive loop.

---

## Sub-Phase 1D: DisplayController Class

**Goal**: LCD 16x2 via I2C showing door status, signal strength, last event.

**Files to create**:
- `include/DisplayController.h`
- `src/DisplayController.cpp`

**Files to modify**:
- `include/SystemController.h` — add `DisplayController m_display` member
- `src/SystemController.cpp` — initialize display, feed it door state and signal quality

**DisplayController class**:
- `begin()` / `loop()` lifecycle
- Content setters: `setDoorState()`, `setSignalQuality()`, `setLastCommandInfo()`, `showMessage()`
- Boot screen: "Garage Monitor" / "Starting..."
- Status screen layout (16 chars per line):
  ```
  Door:OPEN  S:###
  Last: Francesco
  ```
- Signal quality: CSQ value mapped to visual bars
- Graceful handling if LCD not connected (system still works)
- Uses `I2C_ADDR_LCD` (0x27) and `PIN_I2C_SDA/SCL` from Config.h

**Hardware test**: LCD shows boot screen, then live status. Door state updates on reed switch change. Signal bars update periodically.

**Commit boundary**: DisplayController works. SystemController feeds it real data.

---

## Sub-Phase 1E: Command Processing + State Machine Integration

**Goal**: Wire everything together — SMS receive → parse → authorize → execute → respond. Implement the state machine. This is the Phase 1 finish line.

**Files to modify**:
- `include/SystemController.h` — all member objects, state machine fields, new private methods
- `src/SystemController.cpp` — substantial rewrite to orchestrate all subsystems

**SystemController redesign**:
- Members: `ModemHandler`, `DoorSensor`, `MessageParser`, `DisplayController`
- State machine using existing `SystemState` enum: IDLE, ALERT, COMMAND_PROCESSING
- SMS polling every ~5 seconds
- Door alert after `DOOR_ALERT_DELAY_MS` (5 min) — SMS to admin

**SMS processing loop**:
1. Poll `m_modem.checkForSMS()` on interval
2. For each SMS: read → parse with `m_parser` → act on result → delete from SIM
3. Unauthorized sender → silently ignore, delete
4. No permission → reply "Permission denied", delete
5. Authorized → execute command, send response, delete

**Command execution**:
- **STATUS**: Reply with door state, open duration, signal quality
- **CLOSE**: Pulse `PIN_RELAY_CLOSE` for ~500ms, reply with result
- **OPEN**: Pulse `PIN_RELAY_OPEN` for ~500ms (ADMIN only), reply with result

**State machine transitions**:
- IDLE → door open too long → ALERT (send notification to admin)
- ALERT → door closes → IDLE
- ALERT → CLOSE command → activate relay → IDLE
- Any state → SMS received → COMMAND_PROCESSING → back to previous

**Display updates**: door state, signal quality, last command info all fed to DisplayController throughout.

**Hardware test (full end-to-end)**:
1. Boot → LCD shows status
2. Open door → Serial log, display updates
3. Wait 5 min → alert SMS sent to admin phone
4. Send "STATUS" from authorized number → receive status reply
5. Send "CLOSE" from authorized number → relay activates, reply received
6. Send "STATUS" from unknown number → no response (silent ignore)
7. Send "CLOSE" from MONITOR user → "Permission denied" reply
8. Send "OPEN" from CONTROL user → "Permission denied" reply
9. Send "OPEN" from ADMIN → relay activates

**Commit boundary**: Phase 1 complete. Full SMS command loop with security. State machine running. Display showing live status.

---

## Summary

| Sub-Phase | What | Risk | New Files | Effort | Status |
|-----------|------|------|-----------|--------|--------|
| **1A** | SMS Receive (AT commands) | **High** | — | 2-3h | ✅ Done |
| **1B** | DoorSensor class (refactor) | Low | DoorSensor.h/cpp | 1-2h | ✅ Done |
| **1C** | MessageParser + permissions | Medium | MessageParser.h/cpp | 2-3h | ✅ Done |
| **1D** | DisplayController (LCD) | Low | DisplayController.h/cpp | 1-2h | ✅ Done |
| **1E** | Integration + state machine | Medium | — | 3-4h | ✅ Done |

**Recommended order**: 1A → 1B → 1C → 1D → 1E

Each sub-phase leaves the system in a working state and can be tested independently on real hardware.

---

## Progress Log

### 2026-03-02: Sub-Phase 1A Complete
- Added `ReceivedSMS` struct to `ModemHandler.h`
- Implemented `checkForSMS()` via `AT+CMGL="ALL"` with index parsing
- Implemented `readSMS()` via `AT+CMGR=<idx>` with quoted-field parsing (sender, timestamp, body)
- Implemented `deleteSMS()` and `deleteAllSMS()` via `AT+CMGD`
- Added `getSMSIndex()` for accessing parsed indices
- Added SMS polling loop in `SystemController::loop()` (every 5s via `SMS_POLL_INTERVAL_MS`)
- Messages printed to Serial and deleted from SIM after reading
- **Ready for hardware testing**: Send SMS from phone → should appear on Serial monitor → auto-deleted from SIM

### 2026-03-03: Sub-Phases 1B, 1C, 1E Complete + SMS Hardening

**SMS hardening** (ModemHandler + SystemController):
- Changed `AT+CMGL="ALL"` → `AT+CMGL="REC UNREAD"` to prevent reprocessing
  already-read SMS when `deleteSMS()` fails
- Added `deleteReadSMS()` using `AT+CMGD=1,1` (batch-delete all read messages)
- Replaced per-message `deleteSMS(idx)` + retry with single `deleteReadSMS()`
  call after processing loop (N delete commands → 1)
- `deleteAllSMS()` (flag 4) kept for safety-net purge threshold

**Unknown command fix** (SystemController):
- Unknown commands now checked before permission check
- Previously an admin sending "STOP" got "Permission denied" because
  `requiredPermission(UNKNOWN)` returns 0, which always fails permission check
- Now: unknown commands are silently ignored (log only, no SMS wasted)
- "Permission denied" only sent for real commands the user lacks access to

**Door control sequence** (DoorController):
- `close()` and `open()` now do STOP → 500ms pause → CLOSE/OPEN
- Added `RELAY_SEQUENCE_DELAY_MS` constant in Config.h (tunable)
- `close()`/`open()` delegate to `stop()` method instead of calling `pulse()` directly
- Wiring note: STP relay uses COM+NC, SWO/SWC relays use COM+NO (Benincà ZED.24SC)

**Dynamic alert messages** (SystemController + Config.h):
- Added `DOOR_ALERT_DELAY_MIN` constant (user set to 30 min)
- Serial log and SMS alert text derive minutes from config automatically
- `DOOR_ALERT_DELAY_MS` computed from `DOOR_ALERT_DELAY_MIN * 60 * 1000`

**Documentation fixes**:
- Updated all ESP32-C3 references → ESP32 in CLAUDE.md and project plan
- `platformio.ini` already had `board = esp32dev`

**Phase 1 status**: 1D (DisplayController) is the only remaining sub-phase.
All core functionality (door monitoring, SMS command loop, authorization,
relay control) is working and hardware-tested.

### 2026-03-04: Sub-Phase 1D Complete + Phase 2/3 Sensor Integration

**DisplayController** (3-page LCD layout):
- **Page 1 (Network)**: signal quality bars + modem status
- **Page 2 (Environment)**: temperature/humidity (BME280) + water status
- **Page 3 (System)**: door state + uptime display
- FUNC button cycles through pages
- Door open/close LCD notifications (temporary overlay)
- LCD string overflow fixes (all strings fit 16-column display)
- Graceful handling when LCD not connected

**EnvironmentalSensor** (Phase 2 partial):
- BME280 driver integrated, reads temperature and humidity
- Values displayed on LCD environment page
- STATUS SMS includes temp/humidity readings
- Periodic SMS reports intentionally skipped (no unsolicited spam)

**WaterSensor** (Phase 3 complete):
- XKC-Y25 capacitive sensor with software debounce
- Immediate SMS alert on water detection
- Water status shown on LCD environment page

**ButtonController** enhancements:
- CLOSE/OPEN/STOP buttons for manual door control
- FUNC button for display page cycling

**Phase 1 status**: ALL sub-phases (1A–1E) complete. Phase 2/3 sensors
integrated. System is fully functional for door monitoring, SMS command
processing, environmental monitoring, and flood detection.

---

## Future Improvements

### SMS Reception: Polling → URC Notifications (Phase 4)

Current approach polls with `AT+CMGL="REC UNREAD"` every 5 seconds. This works but is wasteful — the modem and UART are busy even when no SMS has arrived, which conflicts with Phase 4 deep sleep goals.

**Better approaches to evaluate in Phase 4:**
1. **URC-based notification** (`AT+CNMI=2,1`): SIM7000G sends an unsolicited `+CMTI` notification the instant an SMS arrives. No polling needed — just listen for the URC on the serial line. Much more efficient for power management.
2. **RI pin hardware interrupt**: SIM7000G can pulse the RI (Ring Indicator) pin on SMS arrival. Wire this to an ESP32 GPIO configured as a wake-up source — the ESP32 can stay in deep sleep and wake only when an SMS actually arrives. Needs a GPIO assignment added to `Config.h`.

**Action items for Phase 4:**
- [ ] Test `AT+CNMI=2,1` URC notification and parse `+CMTI: "SM",<index>` response
- [ ] Assign RI pin GPIO in `Config.h` and test hardware interrupt wake from deep sleep
- [ ] Evaluate whether polling can be eliminated entirely or kept as fallback

### Multilingual SMS Support (Command Aliases + Localized Responses)

Two features to support Italian-speaking users (e.g., family members):

**Command aliases** — allow commands in multiple languages. Minimal change
in `MessageParser::parseCommand()`:
```cpp
if (trimmed == "OPEN" || trimmed == "APRI") return SMSCommand::OPEN;
if (trimmed == "CLOSE" || trimmed == "CHIUDI") return SMSCommand::CLOSE;
if (trimmed == "STATUS" || trimmed == "STATO") return SMSCommand::STATUS;
```

**Localized response phrases** — send SMS replies in the user's preferred
language. Two possible approaches:
1. **Simple `Phrases.h`** — string constants per language, a `language` field
   on `AuthorizedUser`, pick the right string when sending replies.
2. **`getPhrase(PhraseID, Language)` function** — centralizes all translations
   in one place, easier to maintain as phrases grow.

Both allow per-user language preference (Francesco → EN, Papà → IT).

### Architecture: SMS Functions in ModemHandler (Post-Phase 1 Review)

SMS send/receive/delete are currently in `ModemHandler` for pragmatic reasons — it already owns the serial connection and AT command infrastructure. This works well for Phase 1 but doesn't match the planned architecture:

```
CommunicationManager
├── SMSHandler        ← SMS send/receive/delete/storage management
├── ATCommandInterface
└── MessageParser
```

**Current state**: `ModemHandler` handles modem lifecycle (power, init, network) AND SMS operations. As more SMS features are added (multi-part messages, delivery reports, storage management), this class will grow too large.

**Review after Phase 1 completion:**
- [ ] Evaluate whether to extract SMS operations into a dedicated `SMSHandler` class
- [ ] `SMSHandler` would take a reference to `ModemHandler` (or its serial stream) for AT command access
- [ ] `ModemHandler` should ideally be modem lifecycle only: power on/off, SIM unlock, network registration, signal monitoring, raw AT command passthrough
- [ ] Consider whether `CommunicationManager` wrapper is needed or if `SystemController` can coordinate `ModemHandler` + `SMSHandler` directly
