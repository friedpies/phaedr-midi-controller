# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-02-21)

**Core value:** A controller that feels alive — LEDs accurately reflect DAW state, a beat chaser pulses with the music, and every interaction has immediate, satisfying visual feedback.
**Current focus:** Phase 1 — MCU Protocol Foundation

## Current Position

Phase: 1 of 4 (MCU Protocol Foundation)
Plan: 4 of 5 in current phase
Status: In progress
Last activity: 2026-02-22 — Plan 01-04 complete (MCUProtocol SysEx handshake state machine)

Progress: [████░░░░░░] 20%

## Performance Metrics

**Velocity:**
- Total plans completed: 3
- Average duration: 1.3 min
- Total execution time: 4 min

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| 01-mcu-protocol-foundation | 3 | 4 min | 1.3 min |

**Recent Trend:**
- Last 5 plans: 2 min, 1 min, 1 min
- Trend: Fast

*Updated after each plan completion*

## Accumulated Context

### Decisions

From PROJECT.md Key Decisions table (all pending confirmation against Logic Pro):

- MCU over custom CC: Logic has native MCU support; covers transport, faders, knobs, and LED feedback without custom mapping
- Pickup mode (not relative mode): Avoids sudden jumps on bank switch; LED blink gives clear desync feedback
- Beat chaser on grid (not channel buttons): Grid is the visual centerpiece; doesn't conflict with transport state LEDs
- Bottom row = Shift + Record + Stop + Play: Positions 15 & 16 have no LEDs — Stop and Play are appropriate since they don't require LED confirmation

From 01-01 execution:

- No DEBUG guard system: Serial.println lines deleted outright per clean-break decision — no legacy compatibility shims
- Null guard added to handleControlChangeMessage: defensive safety for any future CC number missing from registry

From 01-03 execution:

- Fader uses 14-bit Pitch Bend (not CC) on per-fader MIDI channels 1–8 per MCU protocol; sendPitchBend offset of -8192 required for Teensyduino API compatibility
- Noise threshold of 48 on 14-bit range is proportionally equivalent to Potentiometer's ANALOG_NOISE=3 on 10-bit range
- MCU-06 knob CC reassignment (CC 14-15, 28-31, 118-119 → CC 16-23) deferred to Plan 05 InputManager; Potentiometer class itself is correct as-is

From 01-04 execution:

- Renamed static member SERIAL to DEVICE_SERIAL: Teensy wiring.h defines #define SERIAL 0 which causes macro collision; rename avoids it with no behavior change
- Static challenge bytes 0x7A 0x6B 0x5C 0x4D chosen for initial implementation; randomization deferred to future improvement
- hasTerm=true in all sendSysEx calls: message arrays include F0/F7 delimiters; library does not re-add them

### Pending Todos

None.

### Blockers/Concerns

- **Phase 1 gate (MEDIUM confidence):** MCU SysEx handshake byte sequence is documented from community sources, not official Mackie spec. Must verify against actual Logic Pro behavior with a MIDI monitor before proceeding past Phase 1.
- **Phase 3 note numbers (LOW confidence):** Mode button note assignments in the 69–75 range (CYCLE, CLICK, etc.) use approximate values in some sources. Must capture what Logic actually sends via MIDI monitor before wiring to physical buttons.
- **RESOLVED — FIX-01:** Copy-by-value Button bug in `src/inputManager.cpp` fixed in plan 01-01 (commit e429788).
- **RESOLVED — FIX-02:** Serial.println calls in usbMIDI callbacks removed in plan 01-01 (commit 35d03cb).
- **Pitch bend API offset:** Teensyduino `sendPitchBend` uses -8192 to +8191 (centered at 0); MCU spec uses 0–16383. All fader math must apply the 8192 offset. Confirm installed Teensyduino version before writing fader code.

## Session Continuity

Last session: 2026-02-22
Stopped at: Plan 01-03 complete — Fader class with 14-bit Pitch Bend implemented; Plans 01-01, 01-02, 01-03, 01-04 all complete
Resume file: .planning/phases/01-mcu-protocol-foundation/01-03-SUMMARY.md
