# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-02-21)

**Core value:** A controller that feels alive — LEDs accurately reflect DAW state, a beat chaser pulses with the music, and every interaction has immediate, satisfying visual feedback.
**Current focus:** Phase 1 — MCU Protocol Foundation

## Current Position

Phase: 1 of 4 (MCU Protocol Foundation)
Plan: 0 of TBD in current phase
Status: Ready to plan
Last activity: 2026-02-21 — Roadmap and state initialized

Progress: [░░░░░░░░░░] 0%

## Performance Metrics

**Velocity:**
- Total plans completed: 0
- Average duration: —
- Total execution time: —

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| - | - | - | - |

**Recent Trend:**
- Last 5 plans: —
- Trend: —

*Updated after each plan completion*

## Accumulated Context

### Decisions

From PROJECT.md Key Decisions table (all pending confirmation against Logic Pro):

- MCU over custom CC: Logic has native MCU support; covers transport, faders, knobs, and LED feedback without custom mapping
- Pickup mode (not relative mode): Avoids sudden jumps on bank switch; LED blink gives clear desync feedback
- Beat chaser on grid (not channel buttons): Grid is the visual centerpiece; doesn't conflict with transport state LEDs
- Bottom row = Shift + Record + Stop + Play: Positions 15 & 16 have no LEDs — Stop and Play are appropriate since they don't require LED confirmation

### Pending Todos

None yet.

### Blockers/Concerns

- **Phase 1 gate (MEDIUM confidence):** MCU SysEx handshake byte sequence is documented from community sources, not official Mackie spec. Must verify against actual Logic Pro behavior with a MIDI monitor before proceeding past Phase 1.
- **Phase 3 note numbers (LOW confidence):** Mode button note assignments in the 69–75 range (CYCLE, CLICK, etc.) use approximate values in some sources. Must capture what Logic actually sends via MIDI monitor before wiring to physical buttons.
- **Pre-existing bug (critical):** Copy-by-value Button bug in `src/inputManager.cpp:25` silently breaks all DAW-driven LED updates. This is FIX-01 and must be the very first change made in Phase 1.
- **Pitch bend API offset:** Teensyduino `sendPitchBend` uses -8192 to +8191 (centered at 0); MCU spec uses 0–16383. All fader math must apply the 8192 offset. Confirm installed Teensyduino version before writing fader code.

## Session Continuity

Last session: 2026-02-21
Stopped at: Roadmap and state files created; no implementation work started
Resume file: None
