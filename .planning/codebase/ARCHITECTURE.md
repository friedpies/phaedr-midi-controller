# Architecture

**Analysis Date:** 2026-03-02

## Pattern Overview

**Overall:** Arduino firmware using a hardware abstraction layer (HAL) pattern with event-driven state machines. The firmware is a unidirectional controller that sends input state to Logic Pro DAW and receives feedback via MIDI to update LED displays.

**Key Characteristics:**
- Polled input reading (buttons, knobs, faders) in main loop with debouncing
- USB MIDI bidirectional communication for command/feedback
- MCU protocol handshake for Logic Pro recognition (SysEx-based)
- Asynchronous LED animation engine (ripple effect) running concurrently with input
- Pickup mode and bank switching for fader synchronization with DAW state
- Stateful blink FSM for channel strip button feedback

## Layers

**Hardware Abstraction Layer (Input Devices):**
- Purpose: Encapsulate physical input reading and filtering
- Location: `src/button.h/cpp`, `src/mcuButton.h/cpp`, `src/potentiometer.h/cpp`, `src/fader.h/cpp`
- Contains: `Button` (legacy), `MCUButton` (MCU protocol), `Potentiometer` (knobs/sliders), `Fader` (pitch bend with pickup mode)
- Depends on: Arduino core, Bounce2 (debouncing), SoftPWM (LED control)
- Used by: `InputManager`

**Input Management & Control Layer:**
- Purpose: Central orchestrator for all inputs; polls hardware, manages state machines, routes MIDI output
- Location: `src/inputManager.h/cpp`
- Contains: Input arrays (grid buttons, track buttons, knobs, faders), ripple animation FSM, track selection state, NoteRegistry
- Depends on: All HAL input classes, `NoteRegistry`, `MCUButton`
- Used by: `main.cpp`

**Protocol & Feedback Layer:**
- Purpose: Handles MCU handshake, maps incoming MIDI to hardware feedback
- Location: `src/mcuProtocol.h/cpp`, `src/noteRegistry.h/cpp`, `src/mcuConfig.h`
- Contains: MCU SysEx handshake state machine, note-to-button mapping registry
- Depends on: Arduino core, `MCUButton`
- Used by: `main.cpp`, `InputManager`

**Entry Point & Main Loop:**
- Purpose: Initialize firmware, register MIDI handlers, orchestrate update cycle
- Location: `src/main.cpp`
- Contains: Startup animation, MIDI callback handlers (SysEx, NoteOn/Off, PitchBend), main loop orchestration
- Depends on: All other modules
- Used by: Arduino runtime

## Data Flow

**Input → MIDI Output Path:**

1. `loop()` calls `inputManager.readAll()`
2. For each button (grid, track):
   - `MCUButton::read()` polls Bounce2 debouncer
   - On press (fell), sends `NoteOn(noteNum, 127)` + `NoteOff(noteNum, 0)` to DAW
3. For each knob:
   - `Potentiometer::read()` samples ADC, computes relative delta (MCU VPot format)
   - Sends relative CC (0x01-0x3F for CW, 0x41-0x7F for CCW) on CC 16-23
4. For each fader:
   - `Fader::read()` samples ADC, maps to 14-bit value
   - In SYNCED state: sends `PitchBend` on MIDI channels 1-8
   - In OUT_OF_SYNC (bank switch): waits for crossover, optionally blinks channel button

**MIDI Feedback → LED Update Path:**

1. `usbMIDI.read()` in `loop()` dispatches incoming MIDI to registered handlers
2. `handleNoteOn()` / `handleNoteOff()` → `inputManager.handleNoteMessage()`
3. `inputManager` looks up note in `noteRegistry` → finds corresponding `MCUButton`
4. `MCUButton::setLedState(velocity)` sets SoftPWM brightness (0=off, 127=on, 1=blink FSM)
5. `loop()` calls `inputManager.updateBlinks()` to advance all blink FSMs every iteration

**Fader Feedback → Pickup Mode Path:**

1. `handlePitchBend()` receives 14-bit fader position from Logic on channels 1-8
2. Routes to `Fader::setDawValue()`
3. `Fader` detects bank switch (DAW value far from physical position)
4. If bank switch detected, enters OUT_OF_SYNC state
5. On next `Fader::read()`, lazy-reveals blink on channel button
6. Blink frequency encodes distance to target (600ms far, 200ms close)
7. On crossover, `Fader` transitions to SYNCED, stops blink

**Pre-Handshake Ripple Animation:**

1. Before MCU handshake, `loop()` calls `inputManager.readIdle()` (debounce only)
2. `readIdle()` calls `poll()` on all buttons (no MIDI output, just debounce)
3. On press, triggers `triggerRipple(originIdx)` with LED position map
4. Computes Euclidean distance from origin to all 24 LEDs
5. Stores pre-computed brightness falloff curve
6. `loop()` calls `inputManager.updateRipple()` every iteration
7. Ripple timing: distance-based delay, then hold, then fade (all non-blocking, millis-based)

**State Management:**

- **Button State:** Debounce + LED state only (no toggle—Logic Pro drives LED state via feedback)
- **Fader State:** Last physical reading, last DAW value, pickup mode flag (SYNCED/OUT_OF_SYNC), blink reveal flag
- **Track Selection:** `InputManager._selectedTrack` tracks which fader/knob owns input focus
- **Ripple Animation:** Distance map, brightness map, timing, lit/faded flags per LED
- **MCU Handshake:** Completion flag, retry timer (5s intervals)
- **Blink FSM:** Per-button blink period, phase, last update time

## Key Abstractions

**MCUButton (Multi-Role Button):**
- Purpose: Encapsulate a physical button + optional LED with MCU protocol semantics
- Examples: `gridButtons[0-15]` (K1-K16), `trackButtons[0-7]` (P1-P8)
- Pattern: Dual-mode read (`read()` for MIDI output via note bang, `poll()` for debounce-only); LED state set via `setLedState(velocity)` from NoteRegistry feedback; blink FSM with non-blocking millis-based update

**Fader (MCU Pitch Bend with Pickup):**
- Purpose: Sync hardware fader with DAW value during bank switches, avoid sudden jumps
- Examples: `trackFaders[0-7]`
- Pattern: Two-state machine (SYNCED/OUT_OF_SYNC); lazy blink reveal on first movement; crossover detection with deadband + rail edge cases; distance-encoded blink period; automatic transition back to SYNCED on crossover

**Potentiometer (Dual-Mode Analog Input):**
- Purpose: Read analog knob/slider, output either absolute (7-bit MIDI) or relative (MCU VPot format)
- Examples: `trackKnobs[0-7]` (relative VPot CC 16-23), legacy sliders (absolute)
- Pattern: Threshold-based change detection (ANALOG_NOISE=3), supports inversion (used by knobs), accumulates relative deltas to handle coarse ADC resolution

**NoteRegistry (MIDI Note → Button Mapping):**
- Purpose: Route incoming NoteOn/Off from Logic to correct hardware button for LED feedback
- Examples: Logic sends note 24 (SELECT Ch1) → routes to P1 button → updates LED
- Pattern: `std::map<uint8_t, MCUButton*>` registry; fast O(log n) lookup on every MIDI message

**RippleState (Pre-Handshake Animation):**
- Purpose: Encapsulate 2D ripple wave animation state with distance-based timing
- Pattern: Pre-computed distance/brightness maps (computed once at trigger), non-blocking state tracking (lit/faded flags), time-gated LED updates using `millis()` delta

## Entry Points

**main() / setup():**
- Location: `src/main.cpp:78-91`
- Triggers: Arduino runtime calls setup() once on power-on
- Responsibilities: Initialize InputManager (which init SoftPWM), play startup animation, register MIDI callbacks with usbMIDI, start MCU protocol handshake

**loop():**
- Location: `src/main.cpp:93-104`
- Triggers: Arduino runtime calls continuously
- Responsibilities: Poll all inputs via `inputManager.readAll()` or `inputManager.readIdle()`, update blink FSMs, process MIDI messages, advance MCU handshake retry timer

**MIDI Callbacks:**
- `handleNoteOn()` / `handleNoteOff()`: Route LED feedback to buttons
- `handlePitchBend()`: Route fader DAW value for pickup mode
- `handleSysEx()`: Route to MCU protocol handshake state machine
- Location: `src/main.cpp:49-76`
- Triggers: Teensyduino USB MIDI driver dispatches on message arrival
- Responsibilities: Forward to appropriate InputManager or MCUProtocol method

## Error Handling

**Strategy:** No explicit error handling beyond assertions. Firmware runs to completion or crashes (soft reset via watchdog). Defensive programming used to avoid crashes:
- Range checks on MIDI channel (1-8)
- Sentinel states for button/fader (buttonPin < 0 skips initialization)
- Null checks on button pointers in NoteRegistry

**Patterns:**
- Early return on invalid inputs (e.g., `if (faderIdx < 0 || faderIdx >= NUM_TRACKS) return;`)
- No-op callbacks on unconfigured devices (e.g., `if (_buttonPin < 0) return;`)
- Safe defaults (e.g., ripple stops if `elapsed` exceeds expected range)

## Cross-Cutting Concerns

**Logging:** Serial output disabled in production (comments in `button.cpp` show debug locations); can be re-enabled for troubleshooting via `Serial.println()` and `pio device monitor`

**Validation:**
- MIDI channel range: Logic sends pitch bend on channels 1-8; firmware validates in `handlePitchBend()`
- Note range: No validation (relies on correct NoteRegistry registration)
- CC range: No validation (relies on Potentiometer CC numbers)

**Authentication:** MCU handshake includes static challenge bytes; validation currently disabled (`validateChallengeResponse()` always returns true) due to uncertainty about Logic's response format. Handshake completes immediately (`_handshakeComplete = true` in `begin()`) without waiting for SysEx reply.

**Timing & Concurrency:** All timing is cooperative (millis-based, no interrupts). MIDI callbacks and loop() execute in single thread (no race conditions). Ripple animation uses elapsed time deltas, not blocking delays, to avoid stalling main loop.

---

*Architecture analysis: 2026-03-02*
