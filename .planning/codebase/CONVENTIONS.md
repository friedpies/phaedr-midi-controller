# Coding Conventions

**Analysis Date:** 2026-03-02

## Naming Patterns

**Files:**
- Header files use `.h` extension (not `.hpp`)
- Implementation files use `.cpp` extension
- Names are lowercase with underscores for multi-word files: `inputManager.h`, `buttonRegistry.cpp`, `mcuButton.h`, `pinDefines.h`
- Pin/configuration constants use `pinDefines.h` pattern

**Functions:**
- Member functions: camelCase starting with lowercase: `init()`, `read()`, `setLedState()`, `handleControlChangeMessage()`
- Private helper functions: camelCase: `hasChanged()`, `ledStateToPWM()`, `triggerRipple()`
- Static functions: camelCase: `playStartupAnimation()`, `handleSysEx()`, `handleNoteOn()`
- Callback handlers: `handle[Event]` pattern: `handleSysEx()`, `handleNoteOn()`, `handleNoteOff()`, `handlePitchBend()`

**Variables:**
- Member variables: prefix with underscore `_variableName`: `_buttonPin`, `_ledPin`, `_ccNum`, `_debounceTime`, `_hasLed`, `_ripple`, `_selectedTrack`
- Local variables: camelCase without prefix: `ledState`, `value`, `distance`, `startMs`, `fader14bit`
- Constants: UPPERCASE_WITH_UNDERSCORES: `DEBOUNCE_TIME`, `NUM_GRID_BUTTONS`, `NUM_TRACKS`, `RETRY_INTERVAL_MS`, `LED_MAX_BRIGHTNESS`
- Static compile-time constants defined in headers: `const int NUM_GRID_BUTTONS = 16;`

**Types and Classes:**
- Class names: PascalCase: `Button`, `MCUButton`, `InputManager`, `Potentiometer`, `Fader`, `ButtonRegistry`, `NoteRegistry`, `MCUProtocol`
- Enum names: PascalCase: `PickupState` with UPPERCASE members: `SYNCED`, `OUT_OF_SYNC`
- Struct names: PascalCase: `RippleState`

## Code Style

**Formatting:**
- No automatic formatter detected (no .clang-format, .prettierrc, or eslint config)
- Brace style: Opening brace on same line (1TBS): `if (condition) {`
- Indentation: 4 spaces (observed in all files)
- Line length: Generally follows ~100 character soft limit (observed in comments and code)
- Constructor initialization lists: comma-first on new line when multiple parameters:
  ```cpp
  MCUButton::MCUButton(int buttonPin, int ledPin, int noteNum, int debounceTime)
      : _buttonPin(buttonPin), _ledPin(ledPin), _noteNum(noteNum),
        _debounceTime(debounceTime), _hasLed(ledPin >= 0)
  ```

**Linting:**
- No linter configuration found in repository
- No pre-commit hooks detected
- Manual code review drives style consistency

**Comments:**
- Inline comments use `// Comment` style
- Multi-line comments explained in context
- Block comments above functions explain purpose and parameters
- Design decision markers used: `BOOT-01:`, `PICK-01:`, `MCU-04:`, `MCU-06:` etc. prefix complex sections

## Import Organization

**Order:**
1. Standard/system libraries: `#include <Arduino.h>`, `#include <SoftPWM.h>`, `#include <Bounce2.h>`, `#include <MIDIUSB.h>`
2. Standard C++ libraries: `#include <map>`, `#include <vector>`
3. Local project headers: `#include "pinDefines.h"`, `#include "inputManager.h"`

**Path Aliases:**
- No path aliases detected (not using `#define` or CMake ALIAS patterns)
- Direct relative includes from `src/` directory: `#include "button.h"`

**Header Guards:**
- Format: `#ifndef filename_h` / `#define filename_h` / `#endif`
- Examples: `button_h`, `pin_defines`, `mcu_button_h`, `button_manager` (note: `button_manager` used in `inputManager.h` not standard naming)

## Error Handling

**Patterns:**
- No exceptions (Arduino/embedded C++ best practice — exceptions disabled)
- Sentinel value pattern: `-1` indicates uninitialized/unconfigured state
  - `MCUButton` constructor: `_buttonPin(-1)` signals unused button
  - `Fader`: `_lastFader14bit(-1)` means first read
  - `Potentiometer`: `lastReading = -1` skips first-read send
- Early returns to skip invalid operations:
  ```cpp
  if (_buttonPin < 0) return;  // skip if not configured
  if (!_hasLed) return;        // skip if no LED hardware
  ```
- Range validation with `constrain()` for MIDI values:
  ```cpp
  int absSteps = constrain(abs(steps), 1, 63);  // enforce 1-63 range
  ```
- Ternary operators for conditional assignments:
  ```cpp
  uint8_t msg = (steps > 0) ? (uint8_t)absSteps : (uint8_t)(absSteps | 0x40);
  ```

## Logging

**Framework:** Serial via Arduino `Serial.println()` (commented out in production code)

**Patterns:**
- Debug prints exist but are commented: `// Serial.println("ROSE");` in `button.cpp` line 31
- No active logging in deployed code
- No structured logging or debug levels
- Startup sequence prints expected to be added for handshake debugging if needed

## Comments and Documentation

**When to Comment:**
- Above complex algorithms explaining intent: see `playStartupAnimation()` explaining LED wave sweep in main.cpp
- Explaining non-obvious MIDI behavior:
  ```cpp
  // MCU VPot sign-magnitude relative format (Logic Pro / Mackie Control):
  //   CW:  0x01-0x3F (bit 6 = 0, bits 0-5 = speed 1-63)
  //   CCW: 0x41-0x7F (bit 6 = 1, bits 0-5 = speed 1-63)
  ```
- Documenting pickup/bank-switch thresholds with rationale: `FADER_NOISE_THRESHOLD = 48` comment explains "proportionally equivalent to ANALOG_NOISE=3 on 0-1023 ADC"
- Decision markers linking to design docs: `BOOT-01:`, `PICK-05:`, `MCU-04:` prefix sections

**JSDoc/TSDoc:**
- Not used (embedded C++ with Arduino framework, not TypeScript)
- No formal doc generation tools observed

## Function Design

**Size:**
- Functions generally 10-50 lines
- Complex initialization split into `setup()` (configure) + `init()` (hardware init) pattern
- Example: `MCUButton::setup()` (5 lines, store params), `MCUButton::init()` (10 lines, hardware setup)

**Parameters:**
- Constructor pattern uses member initializer lists: `Button(int buttonPin, int ledPin, int ccNum, int debounceTime)`
- Two-phase construction: default constructor + `setup()` call for arrays
  ```cpp
  MCUButton gridButtons[NUM_GRID_BUTTONS];  // default construct
  gridButtons[0].setup(K1_SW, LED_K1, MCU_NOTE_BANK_LEFT, DEBOUNCE_TIME);  // configure
  ```
- No pointer parameters except for owned objects like `MCUButton* btn` passed to `setChannelButton()`

**Return Values:**
- `bool` for state queries and change detection: `hasChanged()`, `read()`, `poll()`, `isBlinking()`
- `int` for pin/index lookups: `getLEDPin()`, `getNoteNum()`
- `void` for state mutations: `setLedState()`, `startBlink()`, `stopBlink()`
- Sentinel values on failure: `lastReading = -1` means uninitialized

## Module Design

**Exports:**
- Public interface in `.h` files defines API
- Private members in `.h` files hide implementation details:
  ```cpp
  public:
      void init();
      void read();
  private:
      int _buttonPin;
      Bounce _bounce;
  ```
- Extern singletons at file scope: `extern MCUProtocol mcuProtocol;` in `mcuProtocol.h`

**Barrel Files:**
- Not used (not a TypeScript/JavaScript codebase)
- Each `.h` is self-contained; no central export aggregation

## Array and State Management

**Arrays:**
- Fixed-size arrays for hardware inputs: `MCUButton gridButtons[NUM_GRID_BUTTONS];`
- Parallel arrays for spatial data:
  ```cpp
  float dist[24];           // Euclidean distance
  uint8_t brightness[24];   // pre-computed dampened brightness
  bool lit[24];             // has this LED been turned on yet?
  ```
- Maps for sparse lookups: `std::map<uint8_t, MCUButton*> noteToButton;` in `NoteRegistry`

**State Machines:**
- Simple enum-based state: `PickupState { SYNCED, OUT_OF_SYNC };`
- Blink state tracked with `bool _blinking` + `uint32_t _lastBlinkMs` + `bool _blinkPhase`
- Ripple animation state in struct:
  ```cpp
  struct RippleState {
      bool active;
      uint32_t startMs;
      float dist[24];
      // ...
  } _ripple;
  ```

---

*Convention analysis: 2026-03-02*
