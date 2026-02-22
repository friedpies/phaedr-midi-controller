---
phase: 01-mcu-protocol-foundation
plan: "04"
subsystem: midi-protocol
tags: [mcu, sysex, handshake, logic-pro, challenge-response, teensy]

# Dependency graph
requires: []
provides:
  - MCUProtocol class with 4-step Mackie Control Universal SysEx handshake
  - Challenge-response validation using Ardour-verified algorithm
  - 5-second retry timer for unsolicited Host Connection Query
  - isHandshakeComplete() accessor for downstream gating
affects:
  - 01-05 (transport/note mapping gated on handshake completion)
  - Any plan that needs to know whether Logic Pro has recognized the device

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Static class members for constant SysEx byte arrays (MCU_HEADER, DEVICE_SERIAL, CHALLENGE)"
    - "memcmp for SysEx header validation — fast, no allocation"
    - "Retry timer pattern: millis()-based, reset on each outgoing query"

key-files:
  created:
    - src/mcuProtocol.h
    - src/mcuProtocol.cpp
  modified: []

key-decisions:
  - "Device serial renamed SERIAL->DEVICE_SERIAL to avoid Teensy wiring.h #define SERIAL 0 macro collision"
  - "Static challenge bytes 0x7A 0x6B 0x5C 0x4D chosen for simplicity; randomization deferred"
  - "hasTerm=true in all sendSysEx calls — message arrays include F0 and F7 delimiters"

patterns-established:
  - "Challenge-response: expected[0..3] computed from CHALLENGE using Ardour-verified bit ops, masked with 0x7F"
  - "SysEx validation: check complete flag, minimum length, then memcmp header before parsing message type"

requirements-completed: [MCU-01, MCU-02]

# Metrics
duration: 1min
completed: 2026-02-22
---

# Phase 1 Plan 04: MCUProtocol SysEx Handshake Summary

**4-step Mackie Control Universal SysEx handshake state machine with Ardour-verified challenge-response validation and 5-second retry, enabling Logic Pro to recognize the controller as a Mackie Control surface**

## Performance

- **Duration:** ~1 min
- **Started:** 2026-02-22T03:39:31Z
- **Completed:** 2026-02-22T03:40:56Z
- **Tasks:** 1
- **Files modified:** 2

## Accomplishments
- MCUProtocol class created with full 4-step MCU SysEx handshake (Device Query -> Host Connection Query -> Host Connection Reply -> Confirmation)
- Device serial "PHAEDR\0" (0x50 0x48 0x41 0x45 0x44 0x52 0x00) encoded per CONTEXT.md locked decision
- Challenge-response validation algorithm from Ardour open-source implementation verified
- Retry timer: sends unsolicited Host Connection Query on startup and every 5 seconds until handshake completes
- isHandshakeComplete() accessor enables downstream gating in future plans

## Task Commits

Each task was committed atomically:

1. **Task 1: Create MCUProtocol class — SysEx handshake state machine** - `797252a` (feat)

**Plan metadata:** (docs commit — see final_commit below)

## Files Created/Modified
- `src/mcuProtocol.h` - MCUProtocol class declaration with extern global instance
- `src/mcuProtocol.cpp` - Handshake implementation: handleSysEx, sendHostConnectionQuery, sendConfirmation, validateChallengeResponse

## Decisions Made
- Renamed static member `SERIAL` to `DEVICE_SERIAL` to avoid collision with Teensy framework's `#define SERIAL 0` in `wiring.h` — necessary for compilation, no behavior change

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Renamed SERIAL to DEVICE_SERIAL to resolve Teensy macro collision**
- **Found during:** Task 1 (Create MCUProtocol class)
- **Issue:** The Teensy 3.5 framework (`wiring.h` line 129) defines `#define SERIAL 0`. Using `SERIAL` as a static class member name causes the preprocessor to expand it to `0`, producing a compile error: "expected unqualified-id before numeric constant".
- **Fix:** Renamed `static const uint8_t SERIAL[7]` to `static const uint8_t DEVICE_SERIAL[7]` in both the header and implementation. All `memcpy` calls updated accordingly.
- **Files modified:** src/mcuProtocol.h, src/mcuProtocol.cpp
- **Verification:** `pio run` succeeded with `[SUCCESS]` after rename. DEVICE_SERIAL still holds identical byte values (0x50 0x48 0x41 0x45 0x44 0x52 0x00).
- **Committed in:** 797252a (Task 1 commit)

---

**Total deviations:** 1 auto-fixed (Rule 1 - bug/macro collision)
**Impact on plan:** Fix essential for compilation. No behavior change, no scope creep.

## Issues Encountered
- Teensy framework `wiring.h` defines `SERIAL` as a macro; the plan's proposed member name collided with it. Auto-fixed by renaming to `DEVICE_SERIAL`.

## User Setup Required
None - no external service configuration required. Note: Logic Pro must have the device configured in Preferences -> Control Surfaces -> Setup -> New -> Mackie Control before it will send any SysEx (user setup step, not a firmware concern).

## Next Phase Readiness
- MCUProtocol is ready to be wired into main.cpp (begin() in setup(), update() and usbMIDI.read() in loop(), setHandleSystemExclusive forwarding to handleSysEx)
- isHandshakeComplete() enables gating of transport note and fader behavior on a confirmed handshake
- Challenge bytes are static; future improvement could randomize them per-session

---
*Phase: 01-mcu-protocol-foundation*
*Completed: 2026-02-22*
