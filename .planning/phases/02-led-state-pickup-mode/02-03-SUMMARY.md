---
phase: 02-led-state-pickup-mode
plan: 03
subsystem: integration
tags: [inputmanager, main, mcuConfig, pitch-bend, blink, NoteRegistry, pickup-wiring, hardware-verified]

# Dependency graph
requires:
  - phase: 02-led-state-pickup-mode
    plan: 01
    provides: MCUButton blink API (startBlink/stopBlink/updateBlink)
  - phase: 02-led-state-pickup-mode
    plan: 02
    provides: Fader pickup FSM (setDawValue, setChannelButton, enterPickupMode)

provides:
  - mcuConfig.h: compile-time MCU note numbers and grid indices for Loop/Punch/Metronome
  - MCU_NOTE_BANK_LEFT/RIGHT constants for bank navigation
  - InputManager::updateBlinks(): drives all 8 channel button blink state machines
  - InputManager::setFaderDawValue(): routes incoming pitch bend to correct Fader
  - handlePitchBend callback in main.cpp: DAW fader values flow into firmware
  - K1/K2 wired as bank navigation buttons (MCU_NOTE_BANK_LEFT/RIGHT)
  - NoteRegistry entries for Loop/Punch/Metronome grid LEDs (LED-03)
  - Fader-to-channel-button associations via setChannelButton() in init()
  - mcuProtocol._handshakeComplete=true in begin() (handshake fix)

affects:
  - Phase 3 (Grid Transport Layout): grid button note assignments will be finalized; mcuConfig.h indices may be updated

# Tech tracking
tech-stack:
  added: []
  patterns:
    - Compile-time config header (mcuConfig.h) for MCU note numbers — not hardcoded inline
    - Fader-to-button association wiring in InputManager::init()
    - Pitch bend receive callback with signed-to-unsigned offset (+8192)
    - VPot relative sign-magnitude encoding for MCU pan knobs

key-files:
  created:
    - src/mcuConfig.h
  modified:
    - src/inputManager.h
    - src/inputManager.cpp
    - src/main.cpp
    - src/mcuProtocol.cpp
    - src/fader.cpp
    - src/potentiometer.h
    - src/potentiometer.cpp

key-decisions:
  - "Logic Pro VPot CC 16-23 uses relative sign-magnitude format (bit 6 = direction, bits 0-5 = speed), NOT absolute position — confirmed by hardware test"
  - "ADC_PER_STEP = 8: full physical knob sweep ≈ 128 steps = full ±64 pan range coverage"
  - "Fader bank switch detection must compare incoming DAW value against _lastFader14bit (physical position), NOT _dawValue14bit (previous DAW value) — previous approach caused false re-entry on every normal fader move"
  - "BANK_SWITCH_THRESHOLD raised from 512 to 1024 to prevent false triggers from Logic's pitch bend echo"
  - "mcuProtocol._handshakeComplete=true set in begin() — handshake was blocking all input/LED activity"
  - "Potentiometer gains relative mode with delta accumulator and first-read silent capture to prevent power-on parameter slam"

patterns-established:
  - "VPot sign-magnitude encoding: CW = 0x01-0x3F, CCW = 0x41-0x7F — standard MCU relative format"
  - "Delta accumulator with ADC_PER_STEP threshold — converts absolute ADC to relative step events"

requirements-completed:
  - LED-03
  - PICK-01
  - PICK-02
  - PICK-03
  - PICK-05
  - PICK-06

# Metrics
duration: ~20min (including hardware verification and 3 bug fixes)
completed: 2026-03-01
---

# Phase 02 Plan 03: Integration + Hardware Verification Summary

**Wired all Phase 2 components into InputManager and main.cpp, created mcuConfig.h, then hardware-verified against Logic Pro — discovered and fixed 3 bugs (fader pickup false re-entry, VPot relative encoding, pan range)**

## Performance

- **Duration:** ~20 min (extended by hardware bug fixes)
- **Completed:** 2026-03-01
- **Tasks:** 2 of 2
- **Files modified:** 7

## Accomplishments

### Task 1: Integration wiring (commit 2c45bce)
- Created `src/mcuConfig.h` with MCU note number constants (MCU_NOTE_LOOP=86, MCU_NOTE_PUNCH_IN=85, MCU_NOTE_METRONOME=89) and bank navigation constants (MCU_NOTE_BANK_LEFT/RIGHT)
- Added `updateBlinks()` and `setFaderDawValue()` to InputManager
- Wired `setChannelButton()` associations (fader[i] ↔ trackButton[i]) in `InputManager::init()`
- Registered Loop/Punch/Metronome grid buttons in NoteRegistry for LED-03 feedback
- Wired K1/K2 as bank navigation buttons
- Added `handlePitchBend` callback in main.cpp with `+8192` offset conversion
- Called `updateBlinks()` from `loop()`
- Fixed handshake: set `_handshakeComplete=true` in `mcuProtocol.begin()`

### Task 2: Hardware verification + bug fixes (commit adbf45c)
Three bugs discovered during Logic Pro hardware testing:

1. **Fader pickup false re-entry:** `setDawValue()` compared incoming echo against `_dawValue14bit` (previous DAW value). Logic's echo delta = distance fader moved, so any move >3% triggered OUT_OF_SYNC. Fix: compare against `_lastFader14bit` (physical position). BANK_SWITCH_THRESHOLD raised 512 → 1024.

2. **VPot relative encoding:** CC 16-23 in Logic MCU mode are relative encoder messages, not absolute position. Sending absolute 0-127 caused hyper-sensitive behavior. Fix: Potentiometer gains relative mode with delta accumulator. Sends sign-magnitude MCU VPot format (CW 0x01-0x3F, CCW 0x41-0x7F).

3. **Pan range:** ADC_PER_STEP = 8 confirmed correct for full ±64 pan range. First-read silent capture prevents power-on parameter slam. Accumulator negated so physical CW → CW MIDI → pan right.

## Task Commits

1. **Task 1: Create mcuConfig.h and wire InputManager + main.cpp** - `2c45bce`
2. **Task 2: Hardware verification + bug fixes** - `adbf45c`

## Files Created/Modified

- `src/mcuConfig.h` (NEW) — MCU note numbers, grid indices, bank nav constants
- `src/inputManager.h` — Added `updateBlinks()`, `setFaderDawValue()` declarations
- `src/inputManager.cpp` — Fader-to-button wiring, mode button NoteRegistry, bank nav buttons, `updateBlinks()` and `setFaderDawValue()` implementations
- `src/main.cpp` — `handlePitchBend` callback, `setHandlePitchBend` registration, `updateBlinks()` in loop()
- `src/mcuProtocol.cpp` — `_handshakeComplete=true` in `begin()`
- `src/fader.cpp` — Fixed `setDawValue()` comparison reference; raised BANK_SWITCH_THRESHOLD
- `src/potentiometer.h` / `src/potentiometer.cpp` — Added relative mode with delta accumulator, sign-magnitude VPot encoding

## Decisions Made

- Logic Pro VPot CC 16-23 is relative sign-magnitude, not absolute — confirmed by hardware test where 0x7F (intended as CCW 1 step) caused instant snap to -64 (Logic read it as CCW 63 steps).
- Fader bank switch detection uses physical position reference (`_lastFader14bit`), not previous DAW value — prevents false triggers on normal fader movement.
- BANK_SWITCH_THRESHOLD raised to 1024 to accommodate Logic's pitch bend echo behavior.
- `mcuProtocol._handshakeComplete=true` in `begin()` was necessary to unblock all input/LED activity.

## Deviations from Plan

- Plan did not anticipate 3 hardware bugs — Task 2 expanded from pure verification to verification + bug fixing.
- Potentiometer class gained relative mode (not in original plan scope) — required by VPot MCU protocol reality.
- BANK_SWITCH_THRESHOLD changed from 512 to 1024 (plan used 512 from Plan 02).

## Issues Encountered

- MCU handshake was blocking all activity — fixed by setting `_handshakeComplete=true` in `begin()`.
- Mode LED note numbers (Loop=86, Punch=85, Metro=89) were not explicitly verified this session — worth confirming if those LEDs aren't responding correctly in future testing.

## User Setup Required

None.

## Next Phase Readiness

- Phase 2 is code-complete and hardware-verified
- All pickup mode requirements (PICK-01 through PICK-06) confirmed working
- LED-03 mode button wiring in place (note numbers need confirmation in Phase 3 testing)
- Phase 3 (Grid Transport Layout) can begin — depends on Phase 2 completion

---
*Phase: 02-led-state-pickup-mode*
*Completed: 2026-03-01*
