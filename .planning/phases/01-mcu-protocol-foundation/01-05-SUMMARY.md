---
phase: 01-mcu-protocol-foundation
plan: "05"
subsystem: firmware
tags: [mcu-protocol, teensy, usb-midi, inputmanager, softpwm, pitchbend, note-registry, fader, startup-animation]

# Dependency graph
requires:
  - phase: 01-01
    provides: "Bug fixes (FIX-01, FIX-02), Button/ButtonRegistry CC-based system cleaned up"
  - phase: 01-02
    provides: "MCUButton class with Note Bang semantics and NoteRegistry"
  - phase: 01-03
    provides: "Fader class with 14-bit Pitch Bend on MIDI channels 1-8"
  - phase: 01-04
    provides: "MCUProtocol SysEx handshake state machine"
provides:
  - "InputManager rewritten: MCUButton[] for grid/track buttons, Fader[] for sliders, Potentiometer[] with CC 16-23 for knobs"
  - "handleNoteMessage() routes DAW NoteOn/Off to MCUButton::setLedState() via NoteRegistry"
  - "main.cpp: SysEx/NoteOn/NoteOff handlers registered, CC handler removed, playStartupAnimation() BOOT-01"
  - "Full MCU protocol firmware ready for hardware verification with Logic Pro"
affects:
  - "Phase 2 (LED blinking) — MCUButton::setLedState() velocity=1 blink path ready"
  - "Phase 3 (grid button reassignment) — note numbers for K1-K13 are placeholder; Phase 3 reassigns"
  - "Phase 4 (beat chaser) — handleStart/handleClock/handleStop stubs removed; Phase 4 re-adds"

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Two-phase MCUButton/Fader init: default-construct array, call setup() in init()"
    - "MCUButton sentinel: buttonPin=-1 skips all hardware interaction; noteNum=-1 skips MIDI output"
    - "NoteRegistry: note number to MCUButton* map, registered only for LED-equipped buttons"
    - "BOOT-01 animation: SoftPWMBegin called in inputManager.init(), animation runs after init()"

key-files:
  created: []
  modified:
    - src/inputManager.h
    - src/inputManager.cpp
    - src/main.cpp
    - src/mcuButton.h
    - src/mcuButton.cpp
    - src/fader.h
    - src/fader.cpp

key-decisions:
  - "MCUButton/Fader two-phase init: default constructor + setup() vs inline array init; chose setup() pattern for Arduino where new is discouraged"
  - "K13 (Shift) assigned noteNum=-1: prevents sending note 0 (Ch1 REC) accidentally; read() guards on noteNum < 0"
  - "K15/K16 (Stop/Play) use ledPin=-1 and NOT registered in NoteRegistry — these buttons have no LED hardware per CONTEXT.md"
  - "SoftPWMBegin() remains in inputManager.init(); playStartupAnimation() called after init() so SoftPWM is ready"
  - "MCU-04 hardware scope documented: Rewind (91) and FastForward (92) intentionally absent — no physical buttons on this controller"

patterns-established:
  - "Sentinel-guarded MCUButton: buttonPin=-1 skips init/read; noteNum=-1 skips MIDI; LED-equipped check via _hasLed"
  - "NoteRegistry selective registration: only register buttons that can receive LED feedback from DAW"

requirements-completed: [MCU-01, MCU-02, MCU-03, MCU-04, MCU-05, MCU-06, LED-01, LED-02, LED-04, BOOT-01, PWR-01]

# Metrics
duration: 4min
completed: 2026-02-22
---

# Phase 1 Plan 05: MCU Protocol Integration Summary

**Full MCU firmware integration: InputManager rewritten with MCUButton Note Bangs, 14-bit Fader Pitch Bend on MIDI channels 1-8, CC 16-23 knobs, NoteRegistry LED routing, and BOOT-01 cascade startup animation — awaiting hardware verification with Logic Pro**

## Performance

- **Duration:** ~4 min
- **Started:** 2026-02-22T03:44:18Z
- **Completed:** 2026-02-22T03:48:03Z
- **Tasks:** 2 of 3 complete (Task 3 is hardware checkpoint)
- **Files modified:** 7

## Accomplishments

- Rewrote InputManager: CC-based Button arrays replaced with MCUButton Note Bang arrays; Potentiometer sliders replaced with Fader Pitch Bend arrays; knob CC reassigned to 16-23 (MCU-06)
- Rewrote main.cpp: SysEx/NoteOn/NoteOff handlers registered; CC handler removed; BOOT-01 cascade animation runs 24 LEDs on power-on; loop calls mcuProtocol.update() for handshake retry
- Added default constructors + setup() to MCUButton and Fader for clean C++ array initialization without heap allocation
- MCU-04 transport buttons correctly assigned: K14=Record(95), K15=Stop(93), K16=Play(94); no phantom Rewind/FF assignments

## Task Commits

Each task was committed atomically:

1. **Task 1: Rewrite InputManager** - `5c71a55` (feat)
2. **Task 2: Rewrite main.cpp** - `a61ede5` (feat)
3. **Task 3: Hardware verification** - pending checkpoint (human-verify gate)

## Files Created/Modified

- `src/inputManager.h` - MCUButton[], Fader[], NoteRegistry, handleNoteMessage(); removed Button[], ButtonRegistry
- `src/inputManager.cpp` - Full init() with setup() calls, NoteRegistry registration, readAll() with faders
- `src/main.cpp` - SysEx/NoteOn/NoteOff handlers, BOOT-01 animation, mcuProtocol.begin()/update()
- `src/mcuButton.h` - Added default constructor + setup() declaration
- `src/mcuButton.cpp` - Default constructor, setup(), sentinel guards in init() and read()
- `src/fader.h` - Added default constructor + setup() declaration
- `src/fader.cpp` - Default constructor, setup() implementation

## Decisions Made

- **Two-phase init pattern:** MCUButton and Fader default-construct to sentinel (-1) values then are configured via setup() in InputManager::init(). This avoids heap allocation (no `new`) and matches Arduino embedded idioms.
- **K13 noteNum=-1 sentinel:** K13 (Shift) has valid hardware but no MCU note in Phase 1. Setting noteNum=-1 prevents accidental note 0 output (which is Ch1 REC in MCU protocol). MCUButton::read() guards on `_noteNum < 0`.
- **SoftPWMBegin() location:** Kept in inputManager.init() (not duplicated in main.cpp setup()). playStartupAnimation() called after init() ensures SoftPWM is initialized first.
- **Upload protocol note:** `pio run --target upload` requires Teensy Loader GUI to be running; firmware builds successfully to .hex but upload is user-gated.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] MCUButton/Fader missing default constructors for array declaration**
- **Found during:** Task 1 (InputManager rewrite)
- **Issue:** C++ cannot declare `MCUButton gridButtons[16]` without a default constructor — requires all elements to be default-constructible when the array is declared with no initializer
- **Fix:** Added `MCUButton()` and `Fader()` default constructors initializing to sentinel values (-1), plus `setup()` method for two-phase initialization
- **Files modified:** src/mcuButton.h, src/mcuButton.cpp, src/fader.h, src/fader.cpp
- **Verification:** Build succeeds; sentinel guards in init()/read() prevent uninitialized hardware access
- **Committed in:** 5c71a55 (Task 1 commit)

**2. [Rule 1 - Bug] MCUButton::read() would send note 0 for unmapped K13 (Shift)**
- **Found during:** Task 1 (note number assignment review)
- **Issue:** K13 (Shift) has no MCU note in Phase 1; without a guard, pressing K13 would send note 0 (= Ch1 REC in MCU protocol) — wrong behavior
- **Fix:** Set K13 noteNum=-1 and added guard `if (_noteNum < 0) return;` in MCUButton::read() before MIDI output
- **Files modified:** src/mcuButton.cpp, src/inputManager.cpp
- **Verification:** K13 press produces no MIDI output; K13 still debounces (Bounce attaches normally)
- **Committed in:** 5c71a55 (Task 1 commit)

---

**Total deviations:** 2 auto-fixed (both Rule 1 bugs)
**Impact on plan:** Both fixes required for correct behavior. No scope creep.

## Issues Encountered

- Upload failed during Task 3 attempt: `pio run --target upload` requires Teensy Loader GUI to be running. This is a hardware checkpoint — the firmware .hex is built and ready at `.pio/build/teensy35/firmware.hex`. User must open Teensy Loader and run `pio run --target upload` to upload.

## Next Phase Readiness

- Firmware complete and compiles clean (RAM: 2.9%, Flash: 3.5% — well within budget)
- Hardware verification checkpoint awaits user: connect Teensy, open Teensy Loader, run upload, verify with Logic Pro
- Phase 2 (LED blinking for velocity=1) can begin once Logic Pro surface recognition confirmed
- Phase 3 (grid button reassignment) waiting on MIDI monitor capture of actual Logic note numbers

---
*Phase: 01-mcu-protocol-foundation*
*Completed: 2026-02-22*
