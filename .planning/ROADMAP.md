# Roadmap: Phaedr MIDI Controller — MCU Protocol Milestone

## Overview

This milestone converts the existing CC-based Teensy 3.5 firmware into a Mackie Control Universal (MCU) compliant device that Logic Pro recognizes as a native hardware controller. The build moves through four phases gated by a hard dependency chain: the MCU SysEx handshake must succeed before any LED feedback arrives from Logic; bidirectional LED feedback must work before fader pickup behavior can be verified; the transport grid must be mapped and stable before beat-chaser animations can be wired to clock events.

## Phases

- [x] **Phase 1: MCU Protocol Foundation** - Fix the copy-by-value LED bug, complete the MCU SysEx handshake, replace CC sends with MCU Note Bang and Pitch Bend — Logic Pro recognizes the controller and drives LEDs (completed 2026-02-22)
- [x] **Phase 2: LED State + Pickup Mode** - Full LED feedback for loop/punch/metronome grid buttons, per-fader pickup FSM with blink feedback on bank switch

## Phase Details

### Phase 1: MCU Protocol Foundation
**Goal**: Logic Pro recognizes the device as a Mackie Control surface, channel strip buttons and transport buttons drive LEDs bidirectionally, faders send 14-bit pitch bend on per-channel MIDI channels, a cascade animation plays on power-on, and LED brightness is capped within USB power budget
**Depends on**: Nothing (first phase)
**Requirements**: FIX-01, FIX-02, MCU-01, MCU-02, MCU-03, MCU-04, MCU-05, MCU-06, LED-01, LED-02, LED-04, PWR-01, BOOT-01
**Success Criteria** (what must be TRUE):
  1. Plugging in the controller triggers a cascade LED animation (K1→K16, then P1→P8) that completes before any MIDI activity begins
  2. Logic Pro's Control Surfaces preference pane shows "Mackie Control" as an active surface after the Teensy powers on
  3. Pressing a channel strip button (REC/SOLO/MUTE/SELECT) causes Logic to send a Note On back and the corresponding LED on the hardware lights up
  4. Moving any of the 8 sliders causes Logic's on-screen fader to move; a MIDI monitor confirms the message is Pitch Bend (not CC) on the per-fader MIDI channel
  5. Pressing Play, Stop, or Record on the hardware triggers the corresponding transport action in Logic; Logic's response Note On lights the appropriate LED
  6. Plugging in the controller and opening a MIDI monitor shows no debug "CONTROL CHANGE" Serial output interfering with timing
**Plans**: 5 plans

Plans:
- [x] 01-01-PLAN.md — Fix copy-by-value Button bug (FIX-01) and remove Serial.println from MIDI callbacks (FIX-02)
- [x] 01-02-PLAN.md — Create MCUButton class (Note Bang, LED via setLedState with LED_MAX_BRIGHTNESS cap) and NoteRegistry; add LED_MAX_BRIGHTNESS to pinDefines.h (PWR-01)
- [x] 01-03-PLAN.md — Create Fader class (14-bit Pitch Bend per MIDI channel) and document knob CC reassignment
- [x] 01-04-PLAN.md — Create MCUProtocol class (4-step SysEx handshake state machine, retry timer)
- [x] 01-05-PLAN.md — Integration: wire MCUButton, Fader, NoteRegistry, MCUProtocol into InputManager and main.cpp; pulse wave boot animation in setup() (BOOT-01) — hardware verified

### Phase 2: LED State + Pickup Mode
**Goal**: Faders suppress MIDI output after a bank switch until the physical position crosses the DAW value, channel button LEDs blink while a fader is out of sync, and loop/punch/metronome state LEDs reflect real Logic state
**Depends on**: Phase 1
**Requirements**: LED-03, PICK-01, PICK-02, PICK-03, PICK-04, PICK-05, PICK-06
**Success Criteria** (what must be TRUE):
  1. Switching banks in Logic triggers all 8 channel button LEDs to begin blinking simultaneously
  2. Sweeping a fader through its DAW target position stops the blink on that channel's button at the exact crossover point and resumes MIDI output
  3. A fader already at position 0 or 127 when the DAW value is also 0 or 127 picks up immediately without requiring additional physical movement
  4. The loop toggle button LED reflects Logic's cycle mode state (lights when cycle is on, goes dark when cycle is off)
**Plans**: 3 plans

Plans:
- [ ] 02-01-PLAN.md — Add millis()-based blink state machine to MCUButton (PICK-03, PICK-04)
- [ ] 02-02-PLAN.md — Fader pickup FSM: DAW value tracking, MIDI suppression, crossover detection (PICK-01, PICK-02, PICK-04, PICK-05, PICK-06)
- [ ] 02-03-PLAN.md — Integration: InputManager + main.cpp wiring, mcuConfig.h, LED-03 NoteRegistry, hardware verification checkpoint (LED-03, PICK-01, PICK-02, PICK-03, PICK-05, PICK-06)

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. MCU Protocol Foundation | 5/5 | Complete   | 2026-02-22 |
| 2. LED State + Pickup Mode | 3/3 | Complete   | 2026-03-01 |

---
*Roadmap created: 2026-02-21*
*Phase 1 planned: 2026-02-21*
