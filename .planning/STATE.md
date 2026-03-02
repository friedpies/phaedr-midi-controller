# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-02-21)

**Core value:** A controller that feels alive — LEDs accurately reflect DAW state, a beat chaser pulses with the music, and every interaction has immediate, satisfying visual feedback.
**Current focus:** v0.1 cleanup — Phases 1-2 complete, code quality fixes applied

## Current Position

Milestone: v0.1 — Fix Button and Track Activation
Status: In progress — cleaning up hardcoded values, dead code, and planning docs
Last activity: 2026-03-02 — Replace magic numbers with MCU constants, delete unused Button class, trim roadmap

Progress: [████████████████████] 100% (Phases 1-2)

## Accumulated Context

### Decisions

- MCU over custom CC: Logic has native MCU support; covers transport, faders, knobs, and LED feedback without custom mapping
- Pickup mode (not relative mode): Avoids sudden jumps on bank switch; LED blink gives clear desync feedback
- MCUButton is a standalone class (not Button subclass) — old Button class deleted as dead code
- MCU_NOTE_SELECT_BASE, MCU_NOTE_STOP, MCU_NOTE_PLAY, MCU_NOTE_RECORD constants added to mcuConfig.h

### Pending Todos

None.

### Blockers/Concerns

None.

## Session Continuity

Last session: 2026-03-02
Stopped at: v0.1 cleanup complete
