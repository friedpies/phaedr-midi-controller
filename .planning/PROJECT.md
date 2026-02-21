# Phaedr MIDI Controller

## What This Is

Firmware for a Teensy 3.5-based USB MIDI controller with 8 channel strips (knobs, sliders, buttons), a 4×4 button grid, and full bidirectional LED feedback. The controller is being revamped to integrate natively with Logic Pro via the Mackie Control Universal (MCU) protocol, with polished LED animations, a pickup mode for non-motorized pots, and a transport-focused grid layout with shift-layer secondary controls.

## Core Value

A controller that feels alive — LEDs accurately reflect DAW state, a beat chaser pulses with the music, and every interaction has immediate, satisfying visual feedback.

## Requirements

### Validated

<!-- Shipped and confirmed valuable — inferred from existing codebase. -->

- ✓ 8 channel strips (8 rotary knobs, 8 sliders, 8 channel buttons with LEDs) wired and functional — existing
- ✓ 4×4 grid of 16 buttons: 14 with LEDs, 2 without (positions 15 & 16, bottom-right) — existing
- ✓ Bidirectional USB MIDI via Teensy usbMIDI (device → DAW, DAW → device) — existing
- ✓ Bounce2 debouncing on all buttons (20ms window) — existing
- ✓ SoftPWM LED control with fade support — existing
- ✓ ButtonRegistry: CC-to-Button lookup for DAW-driven LED feedback — existing

### Active

<!-- Current scope — what we're building. -->

**Protocol & DAW Integration**
- [ ] Firmware speaks Mackie Control Universal (MCU) MIDI protocol so Logic Pro recognizes it as a native hardware controller
- [ ] Transport callbacks (handleStart, handleStop, handleClock) fully implemented (currently stubs)

**Non-Motorized Pot UX**
- [ ] Pickup mode: fader/knob ignored until physical position crosses through current DAW value — no jumps on bank switch
- [ ] Channel button blinks when its fader is out of sync with DAW; stops blinking once pickup occurs

**Grid Layout — Transport Hub**
- [ ] Grid row 4 (bottom): Shift (btn 13, LED), Record (btn 14, LED), Stop (btn 15, no LED), Play (btn 16, no LED)
- [ ] Grid rows 1–3 (12 buttons, all with LEDs): Bank prev/next, loop toggle, punch in/out, cursor navigation (up/down/left/right), metronome/click toggle, zoom
- [ ] Shift button (btn 13) enables secondary functions when held

**LED State Feedback**
- [ ] LEDs reflect real Logic state: rec arm, mute, solo, loop active, punch active, playing, recording

**Beat Chaser Animation**
- [ ] During playback, a single LED sweeps across the 4×4 grid per beat, synced to MIDI clock from Logic (24 pulses/beat)

**Startup Animations**
- [ ] 4 selectable startup animation styles via a compile-time config variable:
  1. Cascade — LEDs sweep row by row, left to right
  2. Ripple — expand outward from center of grid
  3. Sparkle — random order flicker, settle to correct state
  4. Flash — all LEDs blast on at once, fade to correct state
- [ ] After animation completes, LEDs settle to reflect current DAW state

**Action Animations**
- [ ] Record starts: Record button pulses
- [ ] Bank switch: brief cascade sweep across channel buttons
- [ ] Shift held: subtle dimmed glow across all non-active LEDs

### Out of Scope

- Motorized faders — hardware limitation, not feasible
- RGB / multi-color LEDs — all LEDs are single color
- DIN MIDI — USB MIDI only
- EEPROM persistence — all state is volatile, resets on power cycle
- Ableton Live integration — Logic Pro / Mackie Control only for now

## Context

- Hardware: Teensy 3.5 (ARM Cortex-M4, 72 MHz), Arduino framework, PlatformIO build system
- Libraries: Bounce2 (debouncing), SoftPWM (LED PWM fading), usbMIDI (Teensy built-in)
- All LEDs are a single color (monochromatic — no RGB, animations use brightness/timing only)
- Mackie Control Universal protocol maps faders to pitch bend, knobs to CC 16–23, transport to MIDI notes 91–99; Logic Pro has native MCU support built in
- MIDI clock: Logic sends 24 pulses per beat; beat chaser uses pulse count to drive grid animation
- Existing CC mapping (102–117 grid, 20–27 track, etc.) will be replaced by or mapped alongside MCU protocol messages
- handleStart / handleStop / handleClock are registered but currently empty stubs in main.cpp — the transport and beat animation work goes here

## Constraints

- **Hardware:** Teensy 3.5 only — no cross-platform portability needed
- **Single-color LEDs:** All animations are brightness/timing patterns; no color information available
- **Non-motorized pots:** Physical position can drift from DAW value; pickup mode is the chosen solution
- **USB MIDI:** Controller communicates via USB MIDI only; Mackie Control profile must be selected in Logic's MIDI settings

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Mackie Control Universal (MCU) protocol | Logic Pro has native MCU support; covers transport, faders, knobs, and LED feedback without custom mapping | — Pending |
| Pickup mode for pots (not relative mode) | Avoids sudden jumps; combined with LED blink gives clear user feedback without changing MIDI data model | — Pending |
| Beat chaser on grid (not channel buttons) | Grid is the visual centerpiece; sweeping LEDs there is most visible and doesn't conflict with transport state LEDs | — Pending |
| 4 startup animation styles as config variable | Lets user try options and settle on one without firmware forks | — Pending |
| Bottom row = Shift + Record + Stop + Play | Positions 15 & 16 have no LEDs — Stop and Play are appropriate since they don't require LED confirmation | — Pending |

---
*Last updated: 2026-02-21 after initialization*
