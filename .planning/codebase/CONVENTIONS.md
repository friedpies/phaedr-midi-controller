# Coding Conventions

**Analysis Date:** 2026-02-20

## Naming Patterns

**Files:**
- Header files: `.h` extension (e.g., `button.h`, `inputManager.h`)
- Implementation files: `.cpp` extension (e.g., `button.cpp`, `inputManager.cpp`)
- Pin definitions: `pinDefines.h` follows the convention of a centralized hardware mapping file
- Main entry point: `main.cpp`

**Classes:**
- Pascal case: `Button`, `Potentiometer`, `InputManager`, `ButtonRegistry`
- Singular nouns for entities

**Methods:**
- camelCase: `init()`, `read()`, `setLedState()`, `handleControlChangeMessage()`, `registerButton()`
- Action-oriented names: `init()`, `read()`, `set*`, `handle*`

**Member Variables:**
- Private members prefixed with underscore: `_buttonPin`, `_ledPin`, `_ccNum`, `_debounceTime`, `_pin`, `_invert`, `_ccNum`
- Public member variables (rare): no prefix (e.g., `ledState` in `button.h`)
- Boolean member variables clearly indicate state: `ledState`
- Static constants in UPPER_SNAKE_CASE: `NUM_GRID_BUTTONS`, `NUM_TRACKS`, `DEBOUNCE_TIME`, `READ_RESOLUTION`, `ANALOG_NOISE`

**Constants:**
- Hardware pins: UPPER_SNAKE_CASE prefixed by component: `K1_SW`, `LED_K1`, `KNOB_1`, `SLIDE_1`, `P1_SW`, `LED_P1`
- Semantic constants: `DEFAULT_MIDI_CHANNEL`, `MIDI_VELOCITY_ON` (implicit as 127)

**Local Variables:**
- camelCase: `newButton`, `newValue`, `ccNum`, `velocity`, `mapped`
- Loop counters: single letter `i`

## Code Style

**Formatting:**
- No explicit formatter configured (PlatformIO defaults)
- 4-space indentation observed throughout
- No linter or prettier configuration detected

**Linting:**
- No `.eslintrc`, `.clang-format`, or similar linting configuration found
- Code follows implicit Arduino-style conventions

**File Structure in Headers:**
- Header guard: `#ifndef [name]` / `#define [name]` / `#endif` pattern
- Includes at top: system includes (`<Arduino.h>`), then external libraries (`<Bounce2.h>`), then local headers (`"button.h"`)
- Class declaration follows includes

**File Structure in Implementation:**
- Single include of corresponding header: `#include "button.h"`
- Method implementations follow

## Import Organization

**Order (observed pattern):**
1. System/Framework includes: `<Arduino.h>`
2. External library includes: `<Bounce2.h>`, `<SoftPWM.h>`, `<MIDIUSB.h>`, `<MIDI.h>`, `<map>`
3. Local includes: `"pinDefines.h"`, `"button.h"`, `"inputManager.h"`

**Path Aliases:**
- Not used; all local includes use relative quoted paths: `"button.h"`
- Pin definitions centralized in `pinDefines.h` and included where needed

## Error Handling

**Patterns:**
- No explicit error handling observed
- Silent failures: methods return `void` or primitive types without error codes
- Example: `Potentiometer::init()` is empty and does nothing
- Assumptions made: hardware is always present and functional
- MIDI operations: `usbMIDI.sendControlChange()` called without checking success

**No exception handling:** Arduino embedded context does not use exceptions

## Logging

**Framework:** `Serial` class via `Serial.println()`

**Patterns:**
- Conditional debug logging: `Serial.println("CONTROL CHANGE");` in `main.cpp`
- Some logging is commented out for production: `// Serial.println("HANDLE STOP");`
- Logging appears in:
  - `main.cpp`: `handleControlChangeMessage()`, `handleStart()`, `handleClock()`
  - `potentiometer.cpp`: commented-out debug for `SLIDE_1` readings
  - `button.cpp`: commented-out debug for note on/off events

**When to log:**
- Major state transitions: CONTROL CHANGE, HANDLE START/CLOCK
- Debug information: pin readings (currently commented out)

## Comments

**When to Comment:**
- Hardware-specific warnings: `pinDefines.h` contains comments about pin conflicts and PCB design issues
  - Example: `// MUST REMOVE ONBOARD LED FOR THIS TO WORK`
  - Example: `// BAD SCHEM UD+` indicating PCB errors
  - Example: `// HOOKED UP TO USB NATIVE PORTS, DUMB UD-`
- Disabled features: commented code indicates intentional disablement, not dead code
  - Example: `// void OnNoteOn(byte channel, byte note, byte velocity)` - complete handler definitions kept for reference

**JSDoc/TSDoc:**
- Not used; no formal documentation comment blocks present
- Comments are informal and inline

**Code Comments:**
- Explaining non-obvious logic: `if (button.fell())` followed by state toggle logic
- Explaining mapped values: `map(lastReading, 0, 1023, 0, 127)` converting 10-bit ADC to 7-bit MIDI
- Noting constants: `static const int ANALOG_NOISE = 3; // fluctuation from reading`

## Function Design

**Size:**
- Small, focused functions with single responsibilities
- Examples: `Button::init()` (5 lines), `Button::read()` (19 lines), `Potentiometer::hasChanged()` (2 lines)

**Parameters:**
- Constructor parameters assigned to member variables via initializer list:
  ```cpp
  Button(int buttonPin, int ledPin, int ccNum, int debounceTime) : _buttonPin(buttonPin),
                                                                        _ledPin(ledPin),
                                                                        _ccNum(ccNum),
                                                                        _debounceTime(debounceTime)
  ```
- Method parameters use primitive types and references
- No default parameters in implementations, but observed in declaration: `Potentiometer(int pin, int ccNum, bool invert = false)`

**Return Values:**
- Void methods for state mutations: `init()`, `read()`, `setLedState()`, `handleControlChangeMessage()`
- Primitive returns: `getLEDPin()` returns `int`, `ledStateToPWM()` returns `int`
- Pointer returns for object creation: `registerButton()` returns `Button*`

## Module Design

**Exports (Public Interface):**
- Minimal public API in headers
- Example from `Button`:
  ```cpp
  public:
      void init();
      void read();
      void setLedState(bool newState);
      int getLEDPin();
  ```

**Private Implementation:**
- Member variables are private: `_buttonPin`, `_ledPin`, etc.
- Helper methods are private: `ledStateToPWM()`, `hasChanged()`
- Arduino framework objects as private members: `Bounce button`

**Barrel Files:**
- Not used; no index files aggregating exports

**Initialization Pattern:**
- Constructor initializes member variables
- `init()` method called explicitly in `InputManager::init()` for hardware setup
- Two-phase initialization: construction + initialization

**Data Organization:**
- Arrays as class members: `gridButtons[NUM_GRID_BUTTONS]`, `trackButtons[NUM_TRACKS]`, `trackKnobs[NUM_TRACKS]`, `trackSliders[NUM_TRACKS]` in `InputManager`
- STL containers used: `std::map<int, Button*>` in `ButtonRegistry` for CC-to-button lookup

---

*Convention analysis: 2026-02-20*
