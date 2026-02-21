# Architecture

**Analysis Date:** 2026-02-20

## Pattern Overview

**Overall:** Arduino/Embedded Polling Architecture with Event Delegation

**Key Characteristics:**
- Single-threaded polling loop driven by Arduino's `setup()`/`loop()` pattern
- Hardware abstraction through input abstraction layers (Button, Potentiometer)
- Bidirectional MIDI communication: device → DAW (CC messages), DAW → device (LED feedback via CC handlers)
- Registry pattern for dynamic CC-to-Button mapping enabling DAW-driven LED state updates
- Blocking I/O with debouncing and noise filtering built into input classes

## Layers

**Hardware Abstraction (Input Drivers):**
- Purpose: Encapsulate physical input reading and state management
- Location: `src/button.h/cpp`, `src/potentiometer.h/cpp`
- Contains: Button (debounced switch + LED pair), Potentiometer (analog fader/knob with noise filtering)
- Depends on: Arduino core, Bounce2 (debouncing), SoftPWM (LED PWM control)
- Used by: InputManager

**Input Orchestration:**
- Purpose: Central coordinator for all 40 physical inputs (16 grid buttons, 8 track buttons, 8 knobs, 8 sliders)
- Location: `src/inputManager.h/cpp`
- Contains: Input arrays, initialization logic, CC message routing from DAW
- Depends on: Button, Potentiometer, ButtonRegistry, SoftPWM
- Used by: main.cpp

**Registry & Lookup (Bidirectional Mapping):**
- Purpose: Maps MIDI CC numbers to Button objects, enabling DAW-to-device LED feedback
- Location: `src/buttonRegistry.h/cpp`
- Contains: std::map<int, Button*> for O(1) CC-to-Button lookup
- Depends on: Button
- Used by: InputManager

**Configuration & Pin Definitions:**
- Purpose: Hardware pin constants for all switches, LEDs, and analog inputs
- Location: `src/pinDefines.h`
- Contains: Teensy 3.5 pin assignments (digital, analog, PWM-capable pins)
- Depends on: None
- Used by: Button, Potentiometer, InputManager

**Application Entry Point:**
- Purpose: USB MIDI transport setup, USB-to-firmware message routing
- Location: `src/main.cpp`
- Contains: setup() initialization, loop() orchestration, USB MIDI handler registration
- Depends on: InputManager, Arduino MIDIUSB, platformio build system
- Used by: PlatformIO/Teensy bootloader

## Data Flow

**Hardware Input → DAW (User Pressing Button/Moving Fader):**

1. Main loop calls `inputManager.readAll()` each iteration (non-blocking)
2. InputManager iterates all button/potentiometer arrays, calling `.read()` on each
3. Button.read(): Bounce2 debounces switch, detects falling edge (press), toggles internal LED state
4. Button sends CC message via `usbMIDI.sendControlChange(ccNum, 127 or 0, channel)` to DAW
5. Potentiometer.read(): Reads analog value, compares against last reading with 3-point noise threshold
6. On change, maps 10-bit ADC (0–1023) to 7-bit MIDI (0–127) and sends CC message

**DAW Feedback → Hardware (DAW Updating LED):**

1. DAW sends CC message back to device (e.g., pressing Ableton clip lights LED)
2. Main loop calls `usbMIDI.read()` each iteration (non-blocking)
3. USB handler callback `handleControlChangeMessage()` invoked by usbMIDI
4. Handler delegates to `inputManager.handleControlChangeMessage()`
5. InputManager checks if CC number maps to a button (102–117 grid, 20–27 tracks)
6. Looks up Button* from `buttonRegistry.ccNumToButton[ccNum]`
7. Calls `button.setLedState(velocity == 127 ? HIGH : LOW)`
8. Button updates SoftPWM value (0 or 255) to physically change LED brightness

**State Management:**

- **Button state:** Each Button instance holds `ledState` (boolean). Toggle on press (hardware), overwrite on DAW feedback (software).
- **Potentiometer state:** Each Potentiometer holds `lastReading` (int 0–1023). Compared against new reading with 3-point hysteresis to filter noise.
- **Global state:** InputManager owns all input instances in arrays (grid, track buttons, knobs, sliders).
- **Transient state:** No persistent state written to EEPROM; all state is volatile (resets on power cycle).

## Key Abstractions

**Button (Hardware-Software Bridge):**
- Purpose: Represents a physical momentary switch + LED pair with debouncing and bidirectional state
- Examples: `src/button.h/cpp`, instantiated 24 times in InputManager (16 grid + 8 track buttons)
- Pattern:
  - Constructor stores pin numbers (switch, LED), CC number, debounce time
  - `init()` sets up Bounce2 and SoftPWM
  - `read()` polls Bounce2, detects falling edge, sends CC on press
  - `setLedState()` receives CC feedback from DAW and updates PWM output
  - Maintains internal toggle state (`ledState`) that flips on hardware press but is overwritten on DAW feedback

**Potentiometer (Analog Input with Filtering):**
- Purpose: Reads analog potentiometer/slider with noise rejection and CC mapping
- Examples: `src/potentiometer.h/cpp`, instantiated 16 times (8 knobs, 8 sliders)
- Pattern:
  - Constructor stores analog pin, CC number, optional inversion flag (knobs are inverted)
  - `read()` polls analog input, applies inversion, checks 3-point noise threshold
  - On change, maps 10-bit ADC to 7-bit MIDI using Arduino `map()` function
  - Sends CC message; relies on DAW to display/update parameter

**ButtonRegistry (CC Lookup Table):**
- Purpose: Fast O(1) lookup of Button objects by CC number for DAW feedback routing
- Examples: `src/buttonRegistry.h/cpp`
- Pattern:
  - `registerButton()` factory method creates Button on heap, stores pointer in std::map
  - Enables InputManager to dynamically discover which button owns a given CC number
  - Single instance owned by InputManager

**InputManager (Orchestrator):**
- Purpose: Central hub for input polling, initialization, and message routing
- Examples: `src/inputManager.h/cpp`
- Pattern:
  - Owns arrays of all inputs (16 grid buttons, 8 track buttons, 8 knobs, 8 sliders)
  - Initializes SoftPWM and all Button/Potentiometer instances
  - `readAll()` iterates arrays, calls `.read()` on each (no branching, simple loop)
  - `handleControlChangeMessage()` routes incoming CC from DAW to ButtonRegistry lookup

## Entry Points

**setup():**
- Location: `src/main.cpp` (Arduino standard)
- Triggers: Called once by Teensy bootloader after power-on/reset
- Responsibilities:
  1. Create global InputManager instance
  2. Call `inputManager.init()` to set up all pins, SoftPWM, Bounce2
  3. Register USB MIDI callbacks: `setHandleControlChange()`, `setHandleStart()`, `setHandleClock()`, `setHandleStop()`

**loop():**
- Location: `src/main.cpp` (Arduino standard)
- Triggers: Called repeatedly by Teensy bootloader at full speed (~1000 Hz on Teensy 3.5)
- Responsibilities:
  1. Poll all hardware inputs: `inputManager.readAll()` (reads buttons/knobs/sliders)
  2. Service USB MIDI: `usbMIDI.read()` (checks for incoming CC feedback, invokes callbacks)
  3. Return immediately; blocks for ~5–10 µs total (all reads are non-blocking)

**main.cpp Callback Handlers:**
- `handleControlChangeMessage(byte channel, byte ccNum, byte velocity)`: Routes DAW CC feedback to InputManager
- `handleStart()`, `handleClock()`, `handleStop()`: Placeholders for transport control (currently stub implementations)

## Error Handling

**Strategy:** No explicit error handling. Firmware assumes hardware is always responsive.

**Patterns:**

- **Debounce timeout:** Bounce2 enforces 20 ms debounce interval; if switch bounces during interval, extra edges are ignored (safe).
- **Analog noise:** Potentiometer uses 3-point hysteresis; only sends CC if new reading differs by ≥3 points from last, filtering ADC noise inherently.
- **CC out-of-bounds:** InputManager only updates LEDs for CC 102–117 (grid) and 20–27 (track); other CCs are silently ignored.
- **Missing CC in registry:** If DAW sends CC for a button that wasn't registered, lookup will fail (UB in current code—potential bug).
- **USB disconnection:** Not handled; firmware continues polling, messages are dropped by Teensy USB stack.

## Cross-Cutting Concerns

**Logging:**
- Serial console output via `Serial.println()` in main.cpp callbacks (disabled by default for performance)
- No persistent logging; used for debugging only during development

**Validation:**
- Minimal validation. MIDI values assumed in range (CC 0–127, velocity 0–127, channel 1).
- Analog reads assumed to stay within 0–1023 (would fault if not, but ADC always returns valid 10-bit).
- Button state toggles are unconstrained; no null-safety checks on ButtonRegistry pointers.

**Authentication:**
- Not applicable. Teensy USB MIDI device is always trusted (hardwired, no remote auth).

---

*Architecture analysis: 2026-02-20*
