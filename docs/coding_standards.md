# Coding Standards & Best Practices

> **Purpose**: Maintain consistent, readable, and maintainable code across the entire garage monitoring system project.
> **Applies to**: All C/C++ source files (.cpp, .h), Arduino sketches (.ino)
> **Last Updated**: February 12, 2026

---

## Code Formatting

### Indentation & Spacing
- **Indentation**: 2 spaces (NO tabs)
- **Line endings**: Unix (LF), not Windows (CRLF)
- **Trailing whitespace**: Remove all trailing whitespace
- **Final newline**: Always end files with a newline

```cpp
// ✅ Good - 2 spaces
void setup() {
  Serial.begin(115200);
  delay(1000);
}

// ❌ Bad - 4 spaces or tabs
void setup() {
    Serial.begin(115200);
    delay(1000);
}
```

### Braces & Brackets
- **Opening brace**: Same line as function/method/statement (K&R style)
- **Closing brace**: On its own line
- **Single statement if/for/while**: Use braces anyway for safety

```cpp
// ✅ Good - opening brace on same line
if (condition) {
  doSomething();
}

void myFunction() {
  // code here
}

class MyClass {
public:
  void method() {
    // code here
  }
};

// ❌ Bad - opening brace on new line (Allman style)
if (condition)
{
  doSomething();
}

// ❌ Bad - missing braces
if (condition)
  doSomething();
```

### Line Length
- **Maximum**: 100 characters per line (soft limit)
- **Hard limit**: 120 characters
- Break long lines logically at operators, commas, or parameters

```cpp
// ✅ Good - broken at logical point
const AuthorizedUser AUTHORIZED_USERS[] = {
  {"+39xxxxxxxxxx", PERM_ADMIN, "Francesco"},
  {"+39yyyyyyyyyy", PERM_CONTROL, "Family Member"},
  {nullptr, 0, nullptr}
};

// ✅ Good - long function call broken
systemController->sendSMSNotification(
  phoneNumber,
  message,
  priority
);
```

---

## File Organization

### Header Files (.h)

#### Include Guards
**Always use `#pragma once`**, NOT the traditional macro guards.

```cpp
// ✅ Good - modern and simple
#pragma once

#include <Arduino.h>

class MyClass {
  // ...
};
```

```cpp
// ❌ Bad - old-style macro guards
#ifndef MY_CLASS_H
#define MY_CLASS_H

class MyClass {
  // ...
};

#endif // MY_CLASS_H
```

#### Header Structure
Standard order for header files:

```cpp
#pragma once

// 1. System/Arduino includes
#include <Arduino.h>
#include <Wire.h>

// 2. Third-party library includes
#include <TinyGsmClient.h>

// 3. Project includes
#include "SystemController.h"

// 4. Forward declarations (if needed)
class SomeClass;

// 5. Constants and enums
enum class MyState {
  IDLE,
  ACTIVE
};

// 6. Class declaration
class MyClass {
public:
  // Public interface

private:
  // Private members
};
```

### Implementation Files (.cpp)

#### Implementation Structure
```cpp
// 1. Own header first (catches missing includes)
#include "MyClass.h"

// 2. System/Arduino includes
#include <Arduino.h>

// 3. Other project includes
#include "OtherClass.h"

// 4. Implementation
MyClass::MyClass() {
  // Constructor
}

void MyClass::method() {
  // Implementation
}
```

---

## Naming Conventions

### Files
- **Headers**: PascalCase with `.h` extension → `SystemController.h`
- **Implementation**: PascalCase with `.cpp` extension → `SystemController.cpp`
- **Match class name**: File name should match the primary class name

### Classes & Structs
- **Classes**: PascalCase → `SystemController`, `SMSHandler`
- **Structs**: PascalCase → `AuthorizedUser`, `SensorReading`

### Functions & Methods
- **Functions/Methods**: camelCase → `begin()`, `sendSMS()`, `checkDoorStatus()`
- **Boolean functions**: Prefix with `is`, `has`, `can` → `isDoorOpen()`, `hasSignal()`

### Variables
- **Local variables**: camelCase → `doorState`, `smsMessage`
- **Member variables**: camelCase with `m_` prefix → `m_currentState`, `m_smsHandler`
- **Constants**: UPPER_SNAKE_CASE → `MAX_RETRY_COUNT`, `DEFAULT_TIMEOUT`
- **Enums**: PascalCase for type, UPPER_CASE for values

```cpp
// ✅ Good naming examples
class SystemController {
public:
  void begin();
  bool isDoorOpen();

private:
  SystemState m_currentState;
  int m_retryCount;
};

const int MAX_SMS_LENGTH = 160;

enum class SystemState {
  IDLE,
  MONITORING,
  ALERT
};
```

### Macros & Defines
- **Macros**: UPPER_SNAKE_CASE → `#define DEBUG_ENABLED`
- **Prefer constants over macros** when possible

```cpp
// ✅ Good - const instead of macro
const int SENSOR_PIN = 5;

// ⚠️ OK - but use sparingly
#define DEBUG_MODE 1

// ❌ Avoid - prefer inline functions
#define SQUARE(x) ((x) * (x))
```

---

## Code Style

### Comments

#### File Header Comments
Every source file should have a header comment:

```cpp
/**
 * @file SystemController.cpp
 * @brief Main system coordinator for garage monitoring
 *
 * Manages state machine, coordinates subsystems, and handles
 * lifecycle for the garage monitoring system.
 */
```

#### Class Comments
```cpp
/**
 * @brief Main system coordinator
 *
 * Orchestrates all subsystems (hardware, communication, power)
 * and manages state transitions between IDLE, MONITORING, ALERT, etc.
 */
class SystemController {
  // ...
};
```

#### Function Comments
Use Doxygen-style comments for public methods:

```cpp
/**
 * @brief Send SMS notification to authorized number
 * @param number Phone number to send to (format: +39xxxxxxxxxx)
 * @param message Message content (max 160 chars)
 * @return true if SMS sent successfully, false otherwise
 */
bool sendSMS(const char* number, const char* message);
```

#### Inline Comments
- Use `//` for single-line comments
- Write clear, explanatory comments for complex logic
- Don't state the obvious

```cpp
// ✅ Good - explains WHY
// Wait 50ms to debounce reed switch mechanical bounce
delay(50);

// ❌ Bad - states the obvious
// Delay 50 milliseconds
delay(50);
```

### Whitespace
- **After commas**: Always add a space → `foo(a, b, c)`
- **Around operators**: Add spaces → `x = a + b`, not `x=a+b`
- **Function calls**: No space before parenthesis → `foo()`, not `foo ()`
- **Control statements**: One space before parenthesis → `if (condition)`, not `if(condition)`

```cpp
// ✅ Good spacing
if (x > 0) {
  result = x + y;
  foo(a, b, c);
}

// ❌ Bad spacing
if(x>0){
  result=x+y;
  foo (a,b,c);
}
```

---

## C++ Specific Guidelines

### Modern C++ Features
- **Use `nullptr`** instead of `NULL` or `0` for pointers
- **Use `const`** wherever possible for immutability
- **Use references** instead of pointers when ownership is clear
- **Prefer `enum class`** over plain `enum` for type safety

```cpp
// ✅ Good - modern C++
enum class SystemState {
  IDLE,
  ACTIVE
};

void processData(const SensorData& data) {
  if (data.ptr != nullptr) {
    // process
  }
}

// ❌ Bad - old C style
enum SystemState {
  STATE_IDLE,
  STATE_ACTIVE
};

void processData(SensorData* data) {
  if (data != NULL) {
    // process
  }
}
```

### Memory Management
- **Prefer stack allocation** over heap when possible
- **Use RAII** (Resource Acquisition Is Initialization)
- **Document ownership** clearly in comments
- **Avoid manual `new`/`delete`** when possible (use smart pointers or containers)

```cpp
// ✅ Good - stack allocation
void example() {
  SMSHandler handler;
  handler.begin();
}  // Automatic cleanup

// ⚠️ Use carefully - heap allocation
SystemController* controller = new SystemController();
// Must delete later!
```

### Class Design
- **Initialize in constructor** (use initializer lists when possible)
- **Make methods `const`** if they don't modify state
- **Use access specifiers**: `public`, `private`, `protected` in that order
- **Single responsibility**: Each class should have one clear purpose

```cpp
class SensorReader {
public:
  // Constructor with initializer list
  SensorReader(int pin) : m_pin(pin), m_lastReading(0) {
  }

  // Const method - doesn't modify state
  int getLastReading() const {
    return m_lastReading;
  }

  // Non-const method - modifies state
  void update() {
    m_lastReading = analogRead(m_pin);
  }

private:
  int m_pin;
  int m_lastReading;
};
```

---

## Arduino-Specific Guidelines

### Setup & Loop
- **Keep `main.cpp` minimal**: Just initialization and delegation
- **All logic in classes**: Don't put business logic in `setup()` or `loop()`
- **Use Serial for debugging**: Liberal `Serial.println()` during development

```cpp
// ✅ Good - minimal main.cpp
#include "SystemController.h"

SystemController* systemController = nullptr;

void setup() {
  Serial.begin(115200);
  systemController = new SystemController();
  systemController->begin();
}

void loop() {
  systemController->loop();
  delay(10);
}
```

### Pin Usage
- **Use named constants** for pin numbers
- **Document pin assignments** in header or separate file
- **Group related pins** together

```cpp
// ✅ Good - named constants
const int DOOR_SENSOR_PIN = 5;
const int RELAY_PIN = 6;
const int I2C_SDA = 21;
const int I2C_SCL = 22;

pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);

// ❌ Bad - magic numbers
pinMode(5, INPUT_PULLUP);
```

### String Handling
- **Use `F()` macro** for string literals to save RAM
- **Use `const char*`** for static strings
- **Avoid `String` class** for memory-constrained situations

```cpp
// ✅ Good - strings in flash memory
Serial.println(F("System initialized"));

// ✅ Good - const char* for static strings
const char* DEVICE_NAME = "Garage Monitor";

// ⚠️ Use carefully - String class uses heap
String message = "Status: " + String(value);
```

---

## Security & Safety

### Input Validation
- **Always validate inputs** from external sources (SMS, sensors)
- **Bounds check arrays** before access
- **Sanitize data** before use

```cpp
// ✅ Good - validate before use
bool isAuthorizedNumber(const char* number) {
  if (number == nullptr) return false;
  if (strlen(number) < 10) return false;

  // Check against allowlist
  for (const auto& user : AUTHORIZED_USERS) {
    if (strcmp(number, user.number) == 0) {
      return true;
    }
  }
  return false;
}
```

### Error Handling
- **Check return values** from functions
- **Log errors** via Serial for debugging
- **Fail gracefully**: Don't crash, recover when possible

```cpp
// ✅ Good - check return value
if (!smsHandler.begin()) {
  Serial.println(F("ERROR: SMS Handler init failed"));
  // Enter safe mode or retry
}

// ❌ Bad - ignore return value
smsHandler.begin();  // What if it fails?
```

---

## Documentation

### README Files
- **Every subsystem** should have a brief comment explaining its purpose
- **Complex algorithms** should have explanatory comments
- **Hardware dependencies** should be documented

### Inline Documentation
- **Document assumptions**: "Assumes I2C already initialized"
- **Document side effects**: "Modifies global state"
- **Document thread safety**: "Not thread-safe"

---

## Testing & Debugging

### Debug Output
- **Use consistent prefixes**: `[INFO]`, `[ERROR]`, `[DEBUG]`
- **Include context**: Function name, state, values
- **Remove or guard** verbose debug before production

```cpp
// ✅ Good - clear debug output
Serial.print(F("[INFO] SystemController::begin() - State: "));
Serial.println(static_cast<int>(m_currentState));

// Production: use conditional compilation
#ifdef DEBUG_MODE
  Serial.println(F("[DEBUG] Detailed info here"));
#endif
```

### Assertions
- **Use assertions** for internal invariants during development
- **Remove or disable** in production builds

---

## Performance Guidelines

### Avoid Blocking
- **Don't use long delays** in `loop()`
- **Use millis()** for non-blocking timing
- **Process incrementally** when possible

```cpp
// ✅ Good - non-blocking
unsigned long lastCheck = 0;
const unsigned long CHECK_INTERVAL = 1000;

void loop() {
  unsigned long now = millis();
  if (now - lastCheck >= CHECK_INTERVAL) {
    lastCheck = now;
    checkSensors();
  }
}

// ❌ Bad - blocking
void loop() {
  checkSensors();
  delay(1000);  // Blocks everything!
}
```

### Memory Efficiency
- **Minimize heap usage** (avoid fragmentation)
- **Use flash memory** for constants (`PROGMEM`, `F()` macro)
- **Reuse buffers** instead of allocating repeatedly

---

## Version Control

### Commit Messages
- **Use imperative mood**: "Add feature" not "Added feature"
- **Keep first line short**: <50 characters
- **Explain WHY**: Not just what changed

```
// ✅ Good commit message
Add SMS sender verification with permission system

Implements granular permission model (ADMIN, CONTROL, MONITOR)
to ensure only authorized numbers can send commands. Addresses
security requirement from Phase 1.

// ❌ Bad commit message
Updated code
```

### What to Commit
- **DO commit**: Source code, documentation, configuration
- **DON'T commit**: Build artifacts, IDE settings (except `.vscode/settings.json`), credentials

---

## Configuration Files

### VS Code Settings
See `.vscode/settings.json`:
- Tab size: 2 spaces
- Insert spaces: true
- Trim trailing whitespace: true
- Insert final newline: true

### PlatformIO
See `platformio.ini`:
- Debug level: `CORE_DEBUG_LEVEL=5` for development
- Monitor speed: 115200 baud
- Filters: colorize, esp32_exception_decoder

---

## Review Checklist

Before committing code, verify:

- [ ] Follows 2-space indentation
- [ ] Uses `#pragma once` for headers
- [ ] Opening braces on same line
- [ ] No trailing whitespace
- [ ] Files end with newline
- [ ] Constants are named UPPER_SNAKE_CASE
- [ ] Classes are PascalCase
- [ ] Methods are camelCase
- [ ] Public interface documented
- [ ] Error handling present
- [ ] Debug output clear and useful
- [ ] No blocking delays in loop()
- [ ] Compiles without warnings

---

**Remember**: Consistency is more important than personal preference. Follow these standards even if you prefer a different style.
