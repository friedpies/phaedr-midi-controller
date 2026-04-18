---
phase: 01-mcu-protocol-foundation
plan: "01"
subsystem: firmware
tags: [teensy, arduino, platformio, usb-midi, cpp, led-control]

# Dependency graph
requires: []
provides:
  - "DAW-driven LED updates via Button* pointer (FIX-01)"
  - "Jitter-free MIDI clock callbacks with no Serial output (FIX-02)"
  - "Clean-building firmware gate for MCU protocol work"
affects:
  - 01-mcu-protocol-foundation
  - all subsequent phases that rely on LED feedback

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Button registry uses Button* pointers — always dereference via pointer, never copy-by-value"
    - "usbMIDI callbacks must be lean — no Serial, no blocking ops at 48Hz clock rate"

key-files:
  created: []
  modified:
    - src/inputManager.cpp
    - src/main.cpp

key-decisions:
  - "No DEBUG guard system — Serial.println lines deleted outright per clean-break decision"
  - "Null check added to handleControlChangeMessage for defensive safety (guards missing CC in registry)"

patterns-established:
  - "Button pointer pattern: Button* button = buttonRegistry.ccNumToButton[ccNum]; if (button != nullptr) { button->setLedState(...); }"
  - "Callback hygiene: usbMIDI callbacks delegate immediately, no inline logic or I/O"

requirements-completed: [FIX-01, FIX-02]

# Metrics
duration: 2min
completed: 2026-02-21
---

# Phase 1 Plan 01: Bug Fixes — FIX-01 and FIX-02 Summary

**Fixed copy-by-value Button dereference (FIX-01) and Serial.println jitter in usbMIDI callbacks (FIX-02), unblocking all DAW-driven LED feedback and reliable MIDI clock handling**

## Performance

- **Duration:** ~2 min
- **Started:** 2026-02-22T03:39:18Z
- **Completed:** 2026-02-22T03:40:22Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- FIX-01: Replaced stack-copy Button dereference with `Button*` pointer in `InputManager::handleControlChangeMessage`, so `setLedState()` now mutates the actual Button object stored in `gridButtons[]`/`trackButtons[]` rather than a temporary copy
- FIX-02: Deleted three `Serial.println` calls from `handleControlChangeMessage`, `handleStart`, and `handleClock` — eliminating 1–10ms stalls that caused dropped clock pulses at 24 PPQN (48 fires/sec at 120 BPM)
- Firmware builds clean (`SUCCESS`) with Flash usage slightly reduced (20264 → 18944 bytes) from removing string literals

## Task Commits

Each task was committed atomically:

1. **Task 1: Fix FIX-01 — pointer dereference in handleControlChangeMessage** - `e429788` (fix)
2. **Task 2: Fix FIX-02 — remove Serial.println from all usbMIDI callbacks** - `35d03cb` (fix)

## Files Created/Modified

- `src/inputManager.cpp` - handleControlChangeMessage now uses `Button*` pointer with null guard
- `src/main.cpp` - Three Serial.println calls removed from usbMIDI callback functions

## Decisions Made

- No `#define DEBUG` guard system added — Serial.println lines deleted outright. Per plan's explicit instruction ("Per user decision, clean break; no legacy compatibility shims"). If debug output is needed in future it can be added then.
- Added null guard (`if (button != nullptr)`) before `button->setLedState()` — defensive safety for any future CC number that might not be in the registry.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None — both fixes were straightforward. The `pio` CLI required the full path `/Users/kenmarut/.platformio/penv/bin/pio` since `pio` was not on the shell PATH, but this did not impact any source changes.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Both critical gate bugs are eliminated — DAW-driven LED feedback is now architecturally correct
- Firmware compiles clean and is ready for MCU protocol work (SysEx handshake, transport note handling, fader pitch-bend math) to be layered on top
- No blockers introduced; pre-existing concern about MCU SysEx handshake byte sequence verification against real Logic Pro behavior remains open from STATE.md

---
*Phase: 01-mcu-protocol-foundation*
*Completed: 2026-02-22*

## Self-Check: PASSED

- FOUND: src/inputManager.cpp (modified)
- FOUND: src/main.cpp (modified)
- FOUND: .planning/phases/01-mcu-protocol-foundation/01-01-SUMMARY.md (created)
- FOUND: e429788 (Task 1 commit)
- FOUND: 35d03cb (Task 2 commit)
- FOUND: Button* pointer pattern in inputManager.cpp
- Serial.println count in main.cpp: 1 (commented-out line in handleStop — intentional per plan)
