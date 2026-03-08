# Security Model

## Overview

The garage monitoring system uses **SMS sender verification** to ensure only authorized users can control the door and receive status updates.

## Threat Model

### What We're Protecting Against

✅ **Unauthorized door control** - Random people can't open/close your garage
✅ **Information disclosure** - Only you receive status updates about your garage
✅ **Accidental commands** - Commands from unknown numbers are ignored
✅ **Privacy** - No data sent to cloud services, all local control

### What We're NOT Protecting Against

⚠️ **Physical access to the device** - If someone has physical access, they can reprogram it
⚠️ **SIM card theft** - If someone steals the SIM, they could potentially send SMS (but still need your phone number)
⚠️ **Advanced SMS spoofing** - Very difficult on cellular networks, but theoretically possible

## Authorization Mechanism

### Permission Levels

The system supports **granular permissions** for different users:

| Permission | STATUS | CLOSE | OPEN | CONFIG |
|------------|--------|-------|------|--------|
| **ADMIN** | ✅ | ✅ | ✅ | ✅ |
| **CONTROL** | ✅ | ✅ | ❌ | ❌ |
| **MONITOR** | ✅ | ❌ | ❌ | ❌ |

**Use Cases:**
- **ADMIN**: You (full control)
- **CONTROL**: Family member (can check status and close door, but not open it remotely)
- **MONITOR**: Neighbor checking on garage while you're away (status only, no control)

### Authorization Database (NVS-backed)

Users are stored in ESP32 NVS (non-volatile storage) via `ConfigManager`. On first boot, defaults from `Secrets.h` are seeded into NVS. Users can then be managed via the web UI without reflashing.

```cpp
// Permission flags (Config.h)
enum Permission {
    PERM_STATUS = 0x01,  // Can request status
    PERM_CLOSE  = 0x02,  // Can close door
    PERM_OPEN   = 0x04,  // Can open door
    PERM_CONFIG = 0x08   // Can change settings (+ credit check)
};

// Permission presets
const uint8_t PERM_ADMIN   = PERM_STATUS | PERM_CLOSE | PERM_OPEN | PERM_CONFIG;
const uint8_t PERM_CONTROL = PERM_STATUS | PERM_CLOSE;
const uint8_t PERM_MONITOR = PERM_STATUS;

// Default users seeded from Secrets.h on first boot
const AuthorizedUser DEFAULT_USERS[] = {
    {"+39xxxxxxxxxx", PERM_ADMIN,   "Francesco"},
    {"+39yyyyyyyyyy", PERM_CONTROL, "Family Member"},
    {nullptr, 0, nullptr}  // Terminator
};
```

**NVS storage structure:**
- Namespace `"users"`: phone number, permissions bitmask, display name per user
- Namespace `"settings"`: system configuration values

**How it works:**
1. SMS arrives from `+39yyyyyyyyyy`
2. ConfigManager looks up user by normalized phone number in NVS
3. Checks if user has permission for requested command
4. User has `PERM_CONTROL` → Can STATUS ✅, CLOSE ✅, but not OPEN ❌
5. If no permission → Reply "Permesso negato."
6. If unknown sender → Optionally forward to admins (configurable), then ignore

**User management via web UI:**
- Hold FUNC button at boot → WiFi AP "GarageSetup" activates
- Navigate to web UI → manage users (add/remove/update permissions)
- Changes persist in NVS, survive reboots and firmware updates

## Number Format Normalization

Different carriers may format numbers differently:

```
+39 123 456 7890
0039 123 456 7890
39 123 456 7890
+39123456789
```

**Solution**: Normalize to a canonical format before comparison

```cpp
String normalizePhoneNumber(const String& number) {
    String normalized = number;

    // Remove spaces and dashes
    normalized.replace(" ", "");
    normalized.replace("-", "");

    // Standardize to +39 format
    if (normalized.startsWith("0039")) {
        normalized = "+" + normalized.substring(2);
    } else if (normalized.startsWith("39") && !normalized.startsWith("+39")) {
        normalized = "+" + normalized;
    }

    return normalized;
}
```

## Command Processing Flow

```
┌─────────────────┐
│  SMS Received   │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Extract Sender  │
│    Number       │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│   Normalize     │
│     Format      │
└────────┬────────┘
         │
         ▼
   ┌──────────┐
   │Authorized?│
   └─────┬────┘
         │
    ┌────┴────┐
    │         │
   YES       NO
    │         │
    │         ▼
    │   ┌──────────────┐
    │   │Forward to    │
    │   │admins (if    │
    │   │enabled), then│
    │   │ignore        │
    │   └──────────────┘
    │
    ▼
┌─────────────────┐
│ Parse Command   │
│ STATUS/STATO    │
│ CLOSE/CHIUDI    │
│ OPEN/APRI       │
│ CREDIT/CREDITO  │
└────────┬────────┘
         │
         ▼
   ┌──────────┐
   │Has Perm? │
   └─────┬────┘
         │
    ┌────┴────┐
    │         │
   YES       NO
    │         │
    │         ▼
    │   ┌─────────────┐
    │   │ "Permission │
    │   │   denied"   │
    │   └─────────────┘
    │
    ▼
┌─────────────────┐
│ Execute Action  │
│ (OPEN/CLOSE/    │
│  STATUS)        │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Send Response   │
└─────────────────┘
```

## Supported Commands

### STATUS / STATO
**Requires**: `PERM_STATUS` (all users)
**Action**: Returns current garage status
**Response** (Italian, multi-line):
```
Porta: APERTA (5min)
Temp: 21.5C
Umidita': 65.3%
Acqua: OK
Segnale: **
Attivo da: 2g 3h 45m
```

**Examples:**
- ✅ ADMIN user sends STATUS → Gets status
- ✅ CONTROL user sends STATO → Gets status (Italian alias)
- ✅ MONITOR user sends STATUS → Gets status

### CLOSE / CHIUDI
**Requires**: `PERM_CLOSE` (ADMIN, CONTROL)
**Action**: Triggers door close relay sequence (STOP → pause → CLOSE)
**Response**: "Chiusura porta garage in corso."

**Examples:**
- ✅ ADMIN user sends CLOSE → Door closes
- ✅ CONTROL user sends CHIUDI → Door closes (Italian alias)
- ❌ MONITOR user sends CLOSE → "Permesso negato."

### OPEN / APRI
**Requires**: `PERM_OPEN` (ADMIN only)
**Action**: Triggers door open relay sequence (STOP → pause → OPEN)
**Response**: "Apertura porta garage in corso."

**Examples:**
- ✅ ADMIN user sends OPEN → Door opens
- ❌ CONTROL user sends APRI → "Permesso negato."
- ❌ MONITOR user sends OPEN → "Permesso negato."

### CREDIT / CREDITO
**Requires**: `PERM_CONFIG` (ADMIN only)
**Action**: Sends SMS with "credito" to Iliad service number 400
**Response**: "Richiesta credito inviata al 400."
**Note**: The reply from 400 arrives as an SMS from an unknown sender and is forwarded to admins if forwarding is enabled.

**Examples:**
- ✅ ADMIN user sends CREDITO → Credit inquiry sent
- ❌ CONTROL user sends CREDIT → "Permesso negato."

### Unknown Command
**Action**: None (silently ignored for all users, authorized or not)

### Unknown Sender
**Action**: If `forwardUnknownSms` is enabled, the message is forwarded to all admin users as "INOLTRO da +39xxx:\n<message>". Otherwise silently ignored.

## Why This Is Secure

### 1. Cellular Network Authentication
- Cellular networks validate sender identity
- SMS spoofing is difficult (not impossible, but hard)
- Much harder than IP spoofing on internet

### 2. Allowlist Approach
- Only known-good numbers are accepted
- Default is "deny" (safer than blocklist)
- Easy to audit (just a few numbers)

### 3. No Cloud Dependency
- All processing happens locally on ESP32
- No internet connection required
- Can't be hacked via cloud service vulnerability

### 4. Silent Failures (with optional forwarding)
- Unknown senders get no response
- Optionally, unknown SMS are forwarded to admins for awareness (configurable via web UI)
- Doesn't leak information about valid commands to unknown senders
- No error messages to help attackers

## Future Enhancements

### 1. Secret PIN
Add a PIN requirement for critical commands:

```
OPEN:1234
CLOSE:1234
```

Only authorized numbers with correct PIN execute.

### 2. Command Logging
Log all command attempts (authorized and unauthorized) to a LittleFS-based event log for audit trail. Could share the same ring buffer as the planned system event log, viewable from the web UI diagnostics page.

### 3. Rate Limiting
Limit commands to X per hour to prevent relay wear from accidental spam.

### 4. Tamper Detection
Monitor GPIO for physical tampering, send alert SMS.

## Testing Security

### Test Cases

1. **Authorized number sends CLOSE** → ✅ Should work
2. **Authorized number sends STATUS** → ✅ Should work
3. **Unknown number sends CLOSE** → ❌ Should be ignored (silent)
4. **Unknown number sends STATUS** → ❌ Should be ignored (silent)
5. **Authorized number with typo** → ❌ Should be rejected (log warning)
6. **Authorized number, wrong format** → ✅ Should work (after normalization)

### Verification During Development

```cpp
void testSecurityModel() {
    MessageParser parser;

    // Add authorized number
    parser.addAuthorizedNumber("+39123456789");

    // Test authorized
    assert(parser.isFromAuthorizedNumber("+39123456789") == true);
    assert(parser.isFromAuthorizedNumber("+39 123 456 789") == true);  // Normalized

    // Test unauthorized
    assert(parser.isFromAuthorizedNumber("+39999999999") == false);
    assert(parser.isFromAuthorizedNumber("+1234567890") == false);

    Serial.println("Security tests passed!");
}
```

## Best Practices

### Do:
✅ Use international format (+39...) in authorized list
✅ Test with actual SMS from your phone
✅ Keep authorized numbers list small (2-3 max)
✅ Normalize numbers before comparison
✅ Log unauthorized attempts (for debugging)

### Don't:
❌ Don't send error messages to unauthorized senders
❌ Don't accept commands from any number "temporarily"
❌ Don't hardcode numbers in multiple places (use constants)
❌ Don't forget to test international format variations

## Privacy Considerations

- System never sends data to third parties
- SMS messages stay between you and your garage
- No telemetry, no analytics, no cloud
- You control who has access (via authorized numbers)

## Conclusion

This simple sender-based authentication provides strong security for an IoT device because:

1. **Leverages cellular network security** (hard to spoof)
2. **Simple to implement and audit** (no complex cryptography)
3. **Works offline** (no cloud dependencies)
4. **Fail-safe default** (unknown = denied)

It's not perfect (nothing is), but it's **proportional to the threat model** for a garage door system.

---

**Note**: For critical infrastructure or high-security applications, consider adding cryptographic signatures or challenge-response authentication. For a garage in Venice, sender verification is appropriate and sufficient.
