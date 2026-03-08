# Permission System - Practical Examples

## Quick Reference

| Role | STATUS | CLOSE | OPEN | CONFIG/CREDIT |
|------|--------|-------|------|---------------|
| **ADMIN** | ✅ | ✅ | ✅ | ✅ |
| **CONTROL_FULL** | ✅ | ✅ | ✅ | ❌ |
| **CONTROL** | ✅ | ✅ | ❌ | ❌ |
| **MONITOR** | ✅ | ❌ | ❌ | ❌ |

**Note**: CONTROL_FULL is a custom permission combination (STATUS + CLOSE + OPEN). Permission presets are just convenience — any combination of permission bits can be assigned to a user via the web UI.

## Default Configuration (Secrets.h)

### Step 1: Define Default Users

Default users are defined in `include/Secrets.h` and seeded into NVS on first boot. After that, users are managed via the web UI.

```cpp
// In Secrets.h (not committed to git)
const AuthorizedUser DEFAULT_USERS[] = {
    // Your number - full admin access
    {"+39xxxxxxxxxx", PERM_ADMIN, "Francesco"},

    // Family member - can check status and close door
    {"+39yyyyyyyyyy", PERM_CONTROL, "Mom"},

    // Neighbor - can only check status while you're away
    {"+39zzzzzzzzzz", PERM_MONITOR, "Neighbor Mario"},

    // Terminator
    {nullptr, 0, nullptr}
};
```

After first boot, these users are stored in NVS. To change users without reflashing, use the web UI (hold FUNC at boot → connect to "GarageSetup" WiFi AP → open web UI).

### Step 2: Permission Checking Logic

In `SystemController::handleSMS()`:

```cpp
void SystemController::handleSMS(const ReceivedSMS& sms) {
    // Lookup user in NVS via ConfigManager
    const NvsUser* user = m_config.findUserByPhone(
        MessageParser::normalizeNumber(sms.sender));

    if (!user) {
        // Unknown sender - optionally forward to admins
        if (m_config.getSettings().forwardUnknownSms) {
            notifyAdmins("INOLTRO da " + sms.sender + ":\n" + sms.message);
        }
        return;
    }

    SMSCommand cmd = m_parser.parseCommand(sms.message);

    // Check permission for the command
    if (!m_parser.hasPermission(user->permissions, cmd)) {
        m_modem.sendSMS(sms.sender, "Permesso negato.");
        return;
    }

    // Execute authorized command
    switch (cmd) {
        case SMSCommand::STATUS:
            m_modem.sendSMS(sms.sender, buildStatusReply());
            break;
        case SMSCommand::CLOSE:
            m_door.close();
            m_modem.sendSMS(sms.sender, "Chiusura porta garage in corso.");
            break;
        case SMSCommand::OPEN:
            m_door.open();
            m_modem.sendSMS(sms.sender, "Apertura porta garage in corso.");
            break;
        case SMSCommand::CREDIT:
            m_modem.sendSMS("400", "credito");
            m_modem.sendSMS(sms.sender, "Richiesta credito inviata al 400.");
            break;
        default:
            break;  // Unknown commands silently ignored
    }
}
```

## Real-World Scenarios

### Scenario 1: You (ADMIN) - Full Control

**SMS from +39xxxxxxxxxx:**
```
STATUS
```

**System Response:**
```
Porta: APERTA (5min)
Temp: 21.5C
Umidita': 65.3%
Acqua: OK
Segnale: **
Attivo da: 2g 3h 45m
```

**SMS from +39xxxxxxxxxx:**
```
CHIUDI
```

**System Response:**
```
Chiusura porta garage in corso.
```

**SMS from +39xxxxxxxxxx:**
```
APRI
```

**System Response:**
```
Apertura porta garage in corso.
```

**SMS from +39xxxxxxxxxx:**
```
CREDITO
```

**System Response:**
```
Richiesta credito inviata al 400.
```

---

### Scenario 2: Family Member (CONTROL) - Limited Control

**SMS from +39yyyyyyyyyy (Mom):**
```
STATO
```

**System Response:**
```
Porta: APERTA (5min)
Temp: 21.5C
Umidita': 65.3%
Acqua: OK
Segnale: **
Attivo da: 2g 3h 45m
```

**SMS from +39yyyyyyyyyy (Mom):**
```
CHIUDI
```

**System Response:**
```
Chiusura porta garage in corso.
```

**SMS from +39yyyyyyyyyy (Mom):**
```
APRI
```

**System Response:**
```
Permesso negato.
```

✅ **Why this makes sense**: Mom can check on the garage and close it if needed, but can't remotely open it (security).

---

### Scenario 3: Neighbor (MONITOR) - View Only

**SMS from +39zzzzzzzzzz (Neighbor Mario):**
```
STATO
```

**System Response:**
```
Porta: CHIUSA
Temp: 18.5C
Umidita': 72.1%
Acqua: OK
Segnale: ***
Attivo da: 5g 12h 30m
```

**SMS from +39zzzzzzzzzz (Neighbor Mario):**
```
CHIUDI
```

**System Response:**
```
Permesso negato.
```

**SMS from +39zzzzzzzzzz (Neighbor Mario):**
```
APRI
```

**System Response:**
```
Permesso negato.
```

✅ **Why this makes sense**: Neighbor can check if your garage is secure while you're away on vacation, but can't control it.

---

### Scenario 4: Random Person (UNAUTHORIZED)

**SMS from +39000000000 (Unknown):**
```
APRI
```

**System Response:**
```
(no direct response — message forwarded to admins if forwarding enabled)
```

Admin receives:
```
INOLTRO da +39000000000:
APRI
```

✅ **Why this makes sense**: Unknown senders get no response (don't leak information about valid commands), but admins are notified for awareness.

---

## Permission Bit Flags Explained

### How It Works

```cpp
enum Permission : uint8_t {
    PERM_STATUS = 0x01,  // 0b00000001
    PERM_CLOSE  = 0x02,  // 0b00000010
    PERM_OPEN   = 0x04,  // 0b00000100
    PERM_CONFIG = 0x08   // 0b00001000
};
```

### Combining Permissions (Bitwise OR)

```cpp
// ADMIN has all permissions
PERM_ADMIN = PERM_STATUS | PERM_CLOSE | PERM_OPEN | PERM_CONFIG
           = 0b00001111  // All bits set

// CONTROL has STATUS + CLOSE
PERM_CONTROL = PERM_STATUS | PERM_CLOSE
             = 0b00000011  // First two bits set

// MONITOR has only STATUS
PERM_MONITOR = PERM_STATUS
             = 0b00000001  // Only first bit set
```

### Checking Permissions (Bitwise AND)

```cpp
bool hasPermission(uint8_t userPerms, Permission requiredPerm) {
    return (userPerms & requiredPerm) != 0;
}

// Example: Does CONTROL user have PERM_CLOSE?
userPerms = 0b00000011  (PERM_CONTROL)
requiredPerm = 0b00000010  (PERM_CLOSE)

userPerms & requiredPerm = 0b00000010 (non-zero) → TRUE ✅

// Example: Does CONTROL user have PERM_OPEN?
userPerms = 0b00000011  (PERM_CONTROL)
requiredPerm = 0b00000100  (PERM_OPEN)

userPerms & requiredPerm = 0b00000000 (zero) → FALSE ❌
```

## Adding a New Permission

Want to add a "REBOOT" command for emergency resets?

### Step 1: Add Permission Flag

```cpp
enum Permission : uint8_t {
    PERM_STATUS = 0x01,
    PERM_CLOSE  = 0x02,
    PERM_OPEN   = 0x04,
    PERM_CONFIG = 0x08,
    PERM_REBOOT = 0x10   // NEW: 0b00010000
};
```

### Step 2: Add to ADMIN Preset

```cpp
const uint8_t PERM_ADMIN = PERM_STATUS | PERM_CLOSE | PERM_OPEN | PERM_CONFIG | PERM_REBOOT;
```

### Step 3: Add Command Handler

```cpp
case SMSCommand::REBOOT:
    if (m_parser.hasPermission(user->permissions, SMSCommand::REBOOT)) {
        m_modem.sendSMS(sms.sender, "Sistema in riavvio.");
        ESP.restart();
    }
    // Permission denied handled earlier in the flow
    break;
```

## Changing Permissions

### Option 1: Web UI (Recommended)
Hold FUNC button at boot to enter setup mode. Connect to "GarageSetup" WiFi AP and open `http://192.168.4.1`:

The web UI allows:
- View all users with their phone numbers and permission levels
- Edit permissions for any user
- Add new users
- Remove users

Changes are persisted to NVS immediately and survive reboots.

### Option 2: Change Defaults (Secrets.h)
To change the default users that are seeded on first boot (or after NVS erase), edit `include/Secrets.h`:

```cpp
const AuthorizedUser DEFAULT_USERS[] = {
    {"+39xxxxxxxxxx", PERM_ADMIN,   "Francesco"},
    {"+39yyyyyyyyyy", PERM_ADMIN,   "Mom"},  // Changed from PERM_CONTROL
    {"+39zzzzzzzzzz", PERM_MONITOR, "Neighbor"},
    {nullptr, 0, nullptr}
};
```

**Note**: Changing Secrets.h only affects the initial NVS seed. If NVS already has users, the defaults are not re-applied. To force re-seed, erase NVS first.

## Logging (Optional)

Add logging to track who did what:

```cpp
void logCommand(const String& sender, SMSCommand cmd, bool authorized) {
    String userName = parser.getUserName(sender);
    String cmdName = getCommandName(cmd);

    Serial.print(userName);
    Serial.print(" (");
    Serial.print(sender);
    Serial.print(") ");

    if (authorized) {
        Serial.print("✅ executed: ");
    } else {
        Serial.print("❌ denied: ");
    }

    Serial.println(cmdName);
}
```

**Example Serial Output:**
```
[SMS] Francesco (+39xxxxxxxxxx) -> CHIUDI -> authorized
[SMS] Mom (+39yyyyyyyyyy) -> STATO -> authorized
[SMS] Mom (+39yyyyyyyyyy) -> APRI -> denied
[SMS] Unknown (+39000000000) -> forwarded to admins
```

## Best Practices

### Do:
✅ Give yourself (ADMIN) full permissions
✅ Be conservative with OPEN permission (ADMIN only)
✅ Use CONTROL for trusted family members
✅ Use MONITOR for temporary access (neighbors, house-sitters)
✅ Log permission denials for debugging
✅ Test each permission level thoroughly

### Don't:
❌ Don't give everyone ADMIN access "just in case"
❌ Don't skip permission checks in "trusted" code paths
❌ Don't leak user names/numbers to unauthorized senders
❌ Don't assume permissions - always check explicitly

## Security Note

This permission system is **defense in depth**:

1. **First layer**: Sender number verification (is this person in our list?)
2. **Second layer**: Permission check (can this person do this specific action?)

Even if someone spoofs a phone number (very difficult on cellular), they still can't execute commands they don't have permissions for.

---

**Summary**: The permission system gives you flexible, granular control over who can do what with your garage - from full admin access for you, to view-only monitoring for a neighbor checking while you're away.
