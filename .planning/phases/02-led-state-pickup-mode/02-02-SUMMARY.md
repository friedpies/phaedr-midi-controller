---
phase: 02-led-state-pickup-mode
plan: 02
subsystem: ui
tags: [pickup-mode, FSM, pitch-bend, fader, MCUButton, LED, blink]

# Dependency graph
requires:
  - phase: 02-led-state-pickup-mode
    plan: 01
    provides: MCUButton blink API (startBlink/stopBlink) used by pickup crossover and lazy-reveal

provides:
  - Fader pickup FSM: SYNCED / OUT_OF_SYNC two-state machine per fader
  - setDawValue(): bank switch detection via BANK_SWITCH_THRESHOLD delta guard
  - enterPickupMode(): sets OUT_OF_SYNC, clears lazy-reveal flag (no startBlink — visual silence)
  - read() OUT_OF_SYNC branch: MIDI suppression + lazy blink reveal + crossover detection
  - Crossover with snap pitch bend on sync (PICK-04) and rail edge case (PICK-06)
  - PICKUP_DEADBAND=128, BANK_SWITCH_THRESHOLD=512 (both 14-bit units)

affects:
  - 02-03-PLAN (InputManager wiring: setDawValue() on pitch bend receive, setChannelButton() init, updateBlink() in readAll())

# Tech tracking
tech-stack:
  added: []
  patterns:
    - Two-state FSM (SYNCED/OUT_OF_SYNC) with lazy visual reveal — no noise on bank switch
    - Distance-mapped blink period: map(distance, 0, 16383, 600, 200) encodes proximity to target
    - Rail edge case detection: bothAtZero / bothAtMax guards before deadband check
    - All pickup math in 14-bit units — never mix with 7-bit MIDI or 10-bit ADC values

key-files:
  created: []
  modified:
    - src/fader.h
    - src/fader.cpp

key-decisions:
  - "PICKUP_DEADBAND=128 in 14-bit units: larger than FADER_NOISE_THRESHOLD (48) so ADC jitter cannot cause false pickups, smaller than BANK_SWITCH_THRESHOLD (512)"
  - "enterPickupMode() does NOT call startBlink() — blink is lazily revealed on first significant physical move while OUT_OF_SYNC, per no-visual-noise requirement"
  - "setDawValue() guards on _dawValue14bit >= 0 before delta check — prevents false pickup trigger on first power-on DAW value receipt"
  - "read() updates _lastFader14bit before the OUT_OF_SYNC branch — keeps crossover check stable across loop iterations even during suppressed output"
  - "startBlink() called with distance-mapped period on every significant move in OUT_OF_SYNC — period decreases as fader approaches target (600ms far → 200ms near)"

patterns-established:
  - "Lazy reveal pattern: _blinkRevealed flag set on first startBlink() call in OUT_OF_SYNC; enterPickupMode() resets to false without starting blink"
  - "Snap on crossover: sendPitchBend immediately after SYNCED transition so DAW position snaps to physical fader"

requirements-completed:
  - PICK-01
  - PICK-02
  - PICK-04
  - PICK-05
  - PICK-06

# Metrics
duration: 3min
completed: 2026-02-25
---

# Phase 02 Plan 02: Fader Pickup FSM Summary

**Two-state pickup FSM (SYNCED/OUT_OF_SYNC) added to Fader — suppresses MIDI output after bank switch, lazily reveals distance-encoded blink, and snaps DAW position on crossover**

## Performance

- **Duration:** ~3 min
- **Started:** 2026-02-25T23:17:35Z
- **Completed:** 2026-02-25T23:20:30Z
- **Tasks:** 1 of 1
- **Files modified:** 2

## Accomplishments

- Extended `Fader` class with `PickupState` enum (`SYNCED` / `OUT_OF_SYNC`), private FSM fields (`_dawValue14bit`, `_pickupState`, `_blinkRevealed`, `_channelBtn`), and static constants (`PICKUP_DEADBAND=128`, `BANK_SWITCH_THRESHOLD=512`) — all in 14-bit units
- Added `setChannelButton(MCUButton*)` to wire each fader to its P1–P8 track button
- Implemented `enterPickupMode()`: sets `OUT_OF_SYNC`, clears `_blinkRevealed` — does NOT call `startBlink()` (lazy reveal, no visual noise on bank switch)
- Implemented `setDawValue()`: stores DAW value, detects bank switch via delta > `BANK_SWITCH_THRESHOLD`, guards on `_dawValue14bit >= 0` to skip false trigger on first boot receipt
- Rewrote `read()` with pickup FSM guard: OUT_OF_SYNC branch suppresses `sendPitchBend`, lazily reveals blink with distance-mapped period (`map(distance, 0, 16383, 600, 200)`), detects crossover including rail edge cases (PICK-06), snaps DAW on sync (PICK-04)

## Task Commits

Each task was committed atomically:

1. **Task 1: Add pickup FSM fields and API to Fader** - `1538ff4` (feat)

**Plan metadata commit:** TBD (docs: complete plan)

## Files Created/Modified

- `src/fader.h` — Added `#include "mcuButton.h"`, `PickupState` enum, 3 new public methods, 2 static constants, 4 private FSM fields
- `src/fader.cpp` — Implemented `setChannelButton`, `enterPickupMode`, `setDawValue`; replaced `read()` with pickup-FSM-guarded version

## Decisions Made

- `PICKUP_DEADBAND = 128` in 14-bit units: intentionally larger than `FADER_NOISE_THRESHOLD` (48) to prevent ADC jitter from causing false pickups, while still providing a precise snap feel.
- `enterPickupMode()` does not call `startBlink()` — the lazy reveal pattern ensures no visual noise fires on bank switch. Blink only appears when the user first physically touches the fader.
- `setDawValue()` guards on `_dawValue14bit >= 0` before the delta check — avoids false bank switch trigger on first power-on receipt from the DAW.
- `_lastFader14bit` is updated before the `OUT_OF_SYNC` branch — ensures crossover detection compares a consistent "last seen" value regardless of whether MIDI was suppressed.
- `startBlink()` is called on every significant move in `OUT_OF_SYNC` (not just the first) so the period continuously encodes distance to target, giving the user a visual speedometer as they approach.

## Deviations from Plan

None — plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None — no external service configuration required.

## Next Phase Readiness

- Fader pickup FSM is complete and compilation-verified (`pio run` exits 0)
- Plan 03 (InputManager wiring) must:
  - Call `setChannelButton(P[n])` for each fader during `init()` to associate track buttons
  - Route incoming pitch bend from DAW to `fader.setDawValue()` in the MIDI receive callback
  - Add `mcuButton.updateBlink()` calls in `readAll()` for all 8 track buttons

---
*Phase: 02-led-state-pickup-mode*
*Completed: 2026-02-25*

## Self-Check: PASSED

- FOUND: src/fader.h
- FOUND: src/fader.cpp
- FOUND: .planning/phases/02-led-state-pickup-mode/02-02-SUMMARY.md
- FOUND commit: 1538ff4
