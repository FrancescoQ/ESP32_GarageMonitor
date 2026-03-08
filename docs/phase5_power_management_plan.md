# Phase 5: Power Management — Implementation Plan

## Context

Phases 0-4 are complete. The system runs 24/7 polling sensors and SMS at ~100-180mA total draw. Phase 5 reduces this by putting the SIM7000G modem to sleep between polls (Phase 5a) and optionally putting the ESP32 into deep sleep (Phase 5b).

**Hardware constraints:**
- No RI pin → URC migration (`AT+CNMI`) is NOT possible. SMS polling must stay.
- Door sensor (GPIO 5) is not an RTC GPIO → can't wake from deep sleep (user confirmed this is fine)
- Water sensor (GPIO 32), all buttons (13/14/15/33) ARE RTC GPIOs → can wake from deep sleep

**Approach: Two sub-phases, each independently deployable.**

---

## Phase 5a: Modem DTR Sleep (~60-80% power reduction)

Low-risk, minimal architecture changes. SIM7000G sleeps between SMS polls via DTR pin.

### How it works
- `AT+CSCLK=1` enables DTR sleep mode on the modem
- DTR HIGH → modem sleeps (~1mA), UART inactive, network registration kept
- DTR LOW → modem wakes (~100ms), resumes normal operation
- SMS buffered by network during sleep, retrieved on next poll

### Changes

**`src/ModemHandler.cpp` + `include/ModemHandler.h`**
- Add `enableDTRSleep()` — sends `AT+CSCLK=1`, called at end of `begin()`
- Add `sleep()` — sets DTR HIGH, marks `m_sleeping = true`
- Add `wake()` — sets DTR LOW, waits 100ms, verifies AT response, retry logic
- Add `bool isSleeping()` accessor
- Guard `sendSMS()`, `checkForSMS()`, etc. to auto-wake if called while sleeping

**`src/SystemController.cpp`**
- Wrap SMS poll cycle: `wake()` → poll → process → `sleep()`
- Merge signal quality check into SMS poll window (while modem is already awake)
- Wrap alert sending (water, door, env) with wake/sleep

### Commits (4-5)
1. Add ModemHandler sleep/wake methods (no behavior change yet)
2. Enable DTR sleep in `begin()`, add wake/sleep around SMS poll
3. Add wake/sleep around alert sending (notifyAdmins, etc.)
4. Merge signal check into SMS poll window

---

## Phase 5b: ESP32 Deep Sleep (~95-98% power reduction)

Major architecture change. ESP32 enters deep sleep; wakes on timer or GPIO interrupt. Each wake is a reboot — modem is fast-reconnected (already registered).

### Wake sources

| Source | GPIO/Type | Purpose |
|--------|-----------|---------|
| Timer | RTC timer | Periodic SMS check (default 5 min, configurable) |
| Water | GPIO 32 (EXT1) | Immediate flood alert |
| Buttons | GPIO 13/14/15/33 (EXT1) | Manual door control + display |

Door sensor (GPIO 5) is NOT a wake source — state checked on each wake.

### Boot flow change

```
begin()
  ├── Always first: m_door.begin() (relays fail-safe OFF)
  ├── Cold boot (power-on / reset) → beginFull() [existing logic]
  └── Warm wake (deep sleep) → beginWarmWake(reason)
        ├── Restore state from RTC memory
        ├── Minimal sensor init
        ├── ModemHandler::reconnect() (~500ms vs 15-20s full init)
        ├── Process wake reason (water alert / button / timer)
        ├── Check SMS + sensors + alerts
        ├── Save state → modem.sleep() → deep sleep
        └── Exception: FUNC button → stay awake 30s for LCD interaction
```

### RTC memory (survives deep sleep, not power-off)

```cpp
struct RtcData {
  uint32_t magic;               // 0xDEADBEEF = valid
  uint32_t wakeCount;           // Diagnostics
  uint32_t accumulatedMs;       // Total runtime for uptime/reboot tracking
  bool alertSent;               // Door-open alert suppression
  bool doorWasOpen;             // Door state at last sleep
  bool waterWasDetected;        // Water state at last sleep
  bool tempLowAlertSent;        // Env threshold hysteresis flags
  bool tempHighAlertSent;
  bool humHighAlertSent;
  uint32_t doorOpenStartMs;     // When door opened (accumulated-time), 0 = closed
};
```

### Changes

**New: `include/PowerManager.h` + `src/PowerManager.cpp`**
- `WakeReason` enum (COLD_BOOT, TIMER, WATER_ALERT, BUTTON_*)
- `detectWakeReason()` — reads `esp_sleep_get_wakeup_cause()` + validates RTC magic
- `saveState()` / `getRtcData()` — RTC memory management
- `enterDeepSleep(settings, waterDetected)` — configure EXT1 mask + timer, enter sleep
- Dynamic EXT1 mask: skip GPIOs that are already LOW (prevents instant re-wake)

**`include/ModemHandler.h` + `src/ModemHandler.cpp`**
- Add `reconnect()` — fast re-establish UART + wake DTR + verify network (~500ms)
- Skips PWRKEY toggle, SIM PIN, TinyGSM init, network registration wait

**`include/SystemController.h` + `src/SystemController.cpp`**
- `begin()` dispatches to `beginFull()` (existing) or `beginWarmWake(reason)`
- `beginWarmWake()` — minimal init, process events, check SMS, go back to sleep
- FUNC button wake → stay awake for 30s with LCD, then sleep
- Auto-reboot uses accumulated time from RTC instead of `millis()`

**`include/ConfigManager.h` + `src/ConfigManager.cpp`**
- Add `deepSleepMinutes` to `SystemSettings` (NVS key `"sleep_min"`, default 5)

**`include/Config.h`**
- Add sleep-related constants (DTR wake delay, reconnect timeout, default sleep minutes)

### Commits (6-8)
1. Add PowerManager class with wake reason detection (no deep sleep yet)
2. Add RtcData struct and RTC memory management
3. Add ModemHandler::reconnect() for fast modem reconnect
4. Add deepSleepMinutes to ConfigManager/SystemSettings
5. Refactor SystemController::begin() → beginFull() + beginWarmWake() dispatch
6. Implement beginWarmWake() full wake cycle
7. Implement FUNC button interactive mode (stay awake 30s)
8. Update STATUS reply + auto-reboot to use accumulated uptime

---

## Key trade-offs

- **SMS latency**: With 5-min deep sleep, commands take up to 5 min to process. Configurable via NVS.
- **No URC migration**: Without RI pin, we can't wake on SMS arrival. Polling stays.
- **Door alerts across sleep**: Door state checked on each wake; "open > X min" alert uses accumulated time from RTC memory. May fire slightly late (up to 1 sleep interval).

## Risk mitigations

- **Modem won't wake from DTR**: retry logic + PWRKEY fallback
- **Network lost during sleep**: `reconnect()` checks AT+CREG?, falls back to full `begin()`
- **RTC corruption**: magic number validation, treat as cold boot if invalid
- **Relay safety**: `m_door.begin()` is ALWAYS first call on any wake (fail-safe OFF)
- **GPIO stuck LOW preventing sleep**: dynamic EXT1 mask skips stuck pins

## Testing

User compiles/uploads locally. Key tests per phase:
- **5a**: DTR sleep/wake cycle, SMS delivery after sleep, alert sending during sleep, power measurement
- **5b**: Timer wake, water wake, button wake, door tracking across sleep, cold vs warm boot, modem reconnect failure fallback

## Not included (deferred)

- URC migration (AT+CNMI) — requires RI pin hardware, not available
- BME280 forced mode — negligible savings (~3.6µA normal mode)
- Web UI changes for sleep stats — can be added later
