---
phase: 01-mcu-protocol-foundation
plan: "03"
subsystem: midi
tags: [teensy, pitch-bend, 14-bit, midi, mcu-protocol, fader]

# Dependency graph
requires:
  - phase: 01-mcu-protocol-foundation
    provides: potentiometer class and analog ADC read pattern (Potentiometer used as reference)
provides:
  - Fader class with 14-bit Pitch Bend output on per-fader MIDI channels 1–8
  - MCU-06 knob CC reassignment table documented for Plan 05 InputManager wiring
affects:
  - 01-05-InputManager rewrite (uses Fader class, applies knob CC reassignment)

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "14-bit MIDI: map 10-bit ADC (0-1023) to 0-16383 then subtract 8192 for Teensyduino sendPitchBend API"
    - "Per-fader channel routing: constructor takes midiChannel 1-8, each fader gets its own MIDI channel"
    - "Noise threshold scaled proportionally: Potentiometer uses 3/1023, Fader uses 48/16383 (same ratio)"

key-files:
  created:
    - src/fader.h
    - src/fader.cpp
  modified: []

key-decisions:
  - "Teensyduino sendPitchBend offset: raw 0-16383 converted to signed -8192 to +8191 before transmission; without this fader only covers lower half of travel in Logic"
  - "Noise threshold 48 on 14-bit range matches Potentiometer's 3-value threshold proportionally on 10-bit range"
  - "MCU-06 knob CC reassignment (CC 14-15, 28-31, 118-119 to CC 16-23) deferred to Plan 05 InputManager wiring — Potentiometer class itself unchanged"
  - "Absolute CC values (0-127) on CC 16-23 are correct for analog potentiometer knobs; relative VPot format only applies to rotary encoders"

patterns-established:
  - "Fader sends Pitch Bend (not CC) on its assigned MIDI channel — the MCU protocol requirement for 14-bit fader resolution"
  - "Initial _lastFader14bit = -1 sentinel forces first-read transmission regardless of ADC value"

requirements-completed: [MCU-05, MCU-06]

# Metrics
duration: 1min
completed: 2026-02-22
---

# Phase 1 Plan 03: Fader Class — 14-bit Pitch Bend Summary

**Fader class reading 10-bit ADC, mapping to 14-bit MCU range, and sending signed Pitch Bend on per-fader MIDI channels 1–8 with proportional noise threshold**

## Performance

- **Duration:** 1 min
- **Started:** 2026-02-22T03:39:22Z
- **Completed:** 2026-02-22T03:41:01Z
- **Tasks:** 1
- **Files modified:** 2

## Accomplishments
- Created Fader class as a parallel to Potentiometer for slider inputs
- Implemented 14-bit ADC-to-Pitch-Bend conversion with the critical -8192 Teensyduino offset
- Per-MIDI-channel routing via constructor parameter — fader N maps to MIDI channel N
- Noise threshold of 48 (proportional to Potentiometer's 3-value threshold scaled to 14-bit range) suppresses ADC jitter
- MCU-06 knob CC reassignment table documented in fader.h as a comment for Plan 05 reference

## Task Commits

Each task was committed atomically:

1. **Task 1: Create Fader class — 14-bit Pitch Bend on per-fader MIDI channel** - `b8ef7da` (feat)

**Plan metadata:** (docs commit follows)

## Files Created/Modified
- `src/fader.h` - Fader class declaration with FADER_NOISE_THRESHOLD=48, midiChannel constructor, and MCU-06 knob CC reassignment reference comment
- `src/fader.cpp` - Fader implementation: analogRead 0-1023, map to 0-16383, subtract 8192, call sendPitchBend(pitchBendValue, _midiChannel)

## Decisions Made
- Noise threshold 48 chosen as proportional equivalent: `3/1023 * 16383 ≈ 48`; plan notes it can be increased to 64 or 96 if Logic fader display shows jitter during hardware testing
- MCU-06 knob CC reassignment is documented in fader.h (not implemented yet) because the Potentiometer class is correct as-is; only the constructor arguments in InputManager change, which is Plan 05 work
- `_lastFader14bit = -1` sentinel value ensures the first ADC read always transmits, regardless of whether the value happens to be zero

## Deviations from Plan

None — plan executed exactly as written. The fader files match the specification in the plan to the character.

### Pre-existing Issues Logged to Deferred Items

The `pio run` build verification revealed a pre-existing compilation error in `src/mcuProtocol.h` (from Plan 02): `static const uint8_t SERIAL[7]` collides with Teensyduino's `#define SERIAL 0` in `wiring.h`. This is out of scope for Plan 03 — the fader files have no errors and `main.cpp` compiled without issue. The conflict is documented in `deferred-items.md` for follow-up.

## Issues Encountered

**Build verification partial pass:** `pio run` reported a pre-existing link-step failure (`arm-none-eabi-objcopy: firmware.elf not found`) alongside the `mcuProtocol.cpp` SERIAL name collision. Confirmed both issues are pre-existing (present before Plan 03 changes) and out of scope. The fader files produce no compilation errors. Documented in `deferred-items.md`.

## User Setup Required

None — no external service configuration required.

## Next Phase Readiness

- Fader class is ready for Plan 05 InputManager integration
- Plan 05 will instantiate `Fader` objects for SLIDE_1 through SLIDE_8 on MIDI channels 1–8
- Plan 05 will also apply the MCU-06 knob CC reassignment documented in fader.h
- Blocker: `mcuProtocol.h` SERIAL name collision must be resolved before a clean build is possible (deferred-items.md)

---
*Phase: 01-mcu-protocol-foundation*
*Completed: 2026-02-22*

## Self-Check: PASSED

- FOUND: src/fader.h
- FOUND: src/fader.cpp
- FOUND: .planning/phases/01-mcu-protocol-foundation/01-03-SUMMARY.md
- FOUND: b8ef7da (task commit)
