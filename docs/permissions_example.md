# Permission System - Practical Examples

## Quick Reference

| Role | STATUS | CLOSE | OPEN | CONFIG |
|------|--------|-------|------|--------|
| **ADMIN** | ✅ | ✅ | ✅ | ✅ |
| **CONTROL** | ✅ | ✅ | ❌ | ❌ |
| **MONITOR** | ✅ | ❌ | ❌ | ❌ |

## Phase 1 Setup (Hardcoded)

### Step 1: Define Authorized Users

In `src/communication/MessageParser.cpp` or a dedicated config file:

```cpp
// At the top of the file, after includes
const AuthorizedUser AUTHORIZED_USERS[] = {
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

### Step 2: Permission Checking Logic

In `SystemController::processCommandState()` or SMS handler:

```cpp
void processSMSCommand(const String& sender, SMSCommand cmd) {
    MessageParser parser;

    // Check if sender is authorized at all
    if (!parser.isFromAuthorizedNumber(sender)) {
        // Unknown sender - silently ignore
        return;
    }

    // Process command based on permissions
    switch (cmd) {
        case SMSCommand::STATUS_REQUEST:
            if (parser.hasPermission(sender, PERM_STATUS)) {
                sendStatusSMS(sender);
            } else {
                sendSMS(sender, "Permission denied");
            }
            break;

        case SMSCommand::CLOSE_DOOR:
            if (parser.hasPermission(sender, PERM_CLOSE)) {
                triggerDoorClose();
                sendSMS(sender, "Door close command sent");
            } else {
                sendSMS(sender, "Permission denied: CLOSE requires CONTROL or ADMIN");
            }
            break;

        case SMSCommand::OPEN_DOOR:
            if (parser.hasPermission(sender, PERM_OPEN)) {
                triggerDoorOpen();
                sendSMS(sender, "Door open command sent");
            } else {
                sendSMS(sender, "Permission denied: OPEN requires ADMIN");
            }
            break;

        case SMSCommand::CONFIG_UPDATE:
            if (parser.hasPermission(sender, PERM_CONFIG)) {
                updateConfig(parser.getParameter("key"));
                sendSMS(sender, "Configuration updated");
            } else {
                sendSMS(sender, "Permission denied: CONFIG requires ADMIN");
            }
            break;

        default:
            sendSMS(sender, "Unknown command");
            break;
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
Garage OPEN
Temp: 18.5C, Humidity: 65%
Signal: -67dBm (Good)
Last update: 2 mins ago
```

**SMS from +39xxxxxxxxxx:**
```
CLOSE
```

**System Response:**
```
Door close command sent
Garage is now CLOSED
```

**SMS from +39xxxxxxxxxx:**
```
OPEN
```

**System Response:**
```
Door open command sent
Garage is now OPEN
```

---

### Scenario 2: Family Member (CONTROL) - Limited Control

**SMS from +39yyyyyyyyyy (Mom):**
```
STATUS
```

**System Response:**
```
Garage OPEN
Temp: 18.5C, Humidity: 65%
Signal: -67dBm (Good)
Last update: 2 mins ago
```

**SMS from +39yyyyyyyyyy (Mom):**
```
CLOSE
```

**System Response:**
```
Door close command sent
Garage is now CLOSED
```

**SMS from +39yyyyyyyyyy (Mom):**
```
OPEN
```

**System Response:**
```
Permission denied: OPEN requires ADMIN
```

✅ **Why this makes sense**: Mom can check on the garage and close it if needed, but can't remotely open it (security).

---

### Scenario 3: Neighbor (MONITOR) - View Only

**SMS from +39zzzzzzzzzz (Neighbor Mario):**
```
STATUS
```

**System Response:**
```
Garage OPEN
Temp: 18.5C, Humidity: 65%
Signal: -67dBm (Good)
Last update: 2 mins ago
```

**SMS from +39zzzzzzzzzz (Neighbor Mario):**
```
CLOSE
```

**System Response:**
```
Permission denied: CLOSE requires CONTROL or ADMIN
```

**SMS from +39zzzzzzzzzz (Neighbor Mario):**
```
OPEN
```

**System Response:**
```
Permission denied: OPEN requires ADMIN
```

✅ **Why this makes sense**: Neighbor can check if your garage is secure while you're away on vacation, but can't control it.

---

### Scenario 4: Random Person (UNAUTHORIZED)

**SMS from +39000000000 (Unknown):**
```
STATUS
```

**System Response:**
```
(silent - no response)
```

**SMS from +39000000000 (Unknown):**
```
CLOSE
```

**System Response:**
```
(silent - no response)
```

**SMS from +39000000000 (Unknown):**
```
OPEN
```

**System Response:**
```
(silent - no response)
```

✅ **Why this makes sense**: Unknown senders get no response at all (don't leak information about valid commands).

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
case SMSCommand::REBOOT_SYSTEM:
    if (parser.hasPermission(sender, PERM_REBOOT)) {
        sendSMS(sender, "Rebooting system...");
        ESP.restart();
    } else {
        sendSMS(sender, "Permission denied: REBOOT requires ADMIN");
    }
    break;
```

## Changing Permissions Later

### Phase 1 (Hardcoded)
Edit the code and re-upload firmware:

```cpp
const AuthorizedUser AUTHORIZED_USERS[] = {
    {"+39xxxxxxxxxx", PERM_ADMIN,   "Francesco"},
    {"+39yyyyyyyyyy", PERM_ADMIN,   "Mom"},  // Changed from PERM_CONTROL
    {"+39zzzzzzzzzz", PERM_MONITOR, "Neighbor"},
    {nullptr, 0, nullptr}
};
```

### Phase 5 (Dynamic)
Use WiFi setup mode web interface:

```
http://192.168.4.1/users

Users:
[X] Francesco (+39xxxxxxxxxx) - ADMIN
[X] Mom (+39yyyyyyyyyy) - CONTROL [Edit] [Delete]
[X] Neighbor (+39zzzzzzzzzz) - MONITOR [Edit] [Delete]

[Add New User]
```

Click "Edit" next to Mom, change permissions, save to NVS.

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

**Example Output:**
```
Francesco (+39xxxxxxxxxx) ✅ executed: CLOSE
Mom (+39yyyyyyyyyy) ✅ executed: STATUS
Mom (+39yyyyyyyyyy) ❌ denied: OPEN
Unknown (+39000000000) ❌ rejected: CLOSE (unauthorized)
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
