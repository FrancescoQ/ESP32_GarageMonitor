# Relay Safety: Fail-Safe Boot Behavior

## Core Rule

**Relays must NEVER activate without explicit user action.**

No automatic activation — not at boot, not on power restore, not on sensor
state change, not on timer expiry. The only triggers allowed are:

- SMS command from an authorized user
- Physical button press
- Any future explicit input method

## Boot Behavior

All relay outputs are set to `RELAY_OFF` unconditionally during
`SystemController::begin()`, before any sensor reads or modem
initialization. This is the very first GPIO action after pin mode setup.

```cpp
// Correct — always safe
pinMode(PIN_RELAY_CLOSE, OUTPUT);
digitalWrite(PIN_RELAY_CLOSE, RELAY_OFF);
pinMode(PIN_RELAY_STOP, OUTPUT);
digitalWrite(PIN_RELAY_STOP, RELAY_OFF);
pinMode(PIN_RELAY_OPEN, OUTPUT);
digitalWrite(PIN_RELAY_OPEN, RELAY_OFF);
```

## Why This Matters

**Power cycle scenario:**

1. Door is open
2. Power is lost (outage, brown-out, accidental unplug)
3. Power returns, ESP32 reboots
4. **Without fail-safe**: relay energizes based on door state — door moves
   with nobody present, no warning, potential injury or property damage
5. **With fail-safe**: all relays stay OFF — system waits for explicit
   human command before moving anything

## Prohibited Patterns

These patterns are **never acceptable** in any code path:

```cpp
// DANGEROUS — relay activated by sensor state
digitalWrite(PIN_RELAY_CLOSE, m_doorSensor.isDoorOpen() ? RELAY_ON : RELAY_OFF);

// DANGEROUS — relay activated by callback on state change
m_doorSensor.onStateChange([](bool doorOpen) {
  digitalWrite(PIN_RELAY_CLOSE, doorOpen ? RELAY_ON : RELAY_OFF);
});

// DANGEROUS — relay activated by timer
if (doorOpenDuration > SOME_THRESHOLD) {
  digitalWrite(PIN_RELAY_CLOSE, RELAY_ON);
}
```

## Code Review Checklist

Any PR that touches relay GPIOs (`PIN_RELAY_CLOSE`, `PIN_RELAY_STOP`,
`PIN_RELAY_OPEN`) must verify:

- [ ] `begin()` sets all relays to `RELAY_OFF` unconditionally
- [ ] No callback or sensor reading activates a relay automatically
- [ ] `RELAY_ON` only appears inside explicit-action handlers (SMS command
      processing, button press handlers)
- [ ] No timer or threshold triggers relay activation
- [ ] Power-cycle scenario remains safe (boot → all OFF → wait for command)

## Grep Verification

Quick check that no unsafe relay activation exists outside of command handlers:

```bash
grep -rn "RELAY_ON" src/
```

`RELAY_ON` should only appear in:
- SMS command handlers (e.g., processing a CLOSE or OPEN command)
- Physical button handlers
- `Config.h` (constant definition)
