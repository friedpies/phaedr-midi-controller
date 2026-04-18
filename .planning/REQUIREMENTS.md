# Requirements: Phaedr MIDI Controller

**Defined:** 2026-02-21
**Core Value:** A controller that feels alive — LEDs accurately reflect DAW state, a beat chaser pulses with the music, and every interaction has immediate, satisfying visual feedback.

## v1 Requirements

### MCU Protocol Foundation

- [x] **MCU-01**: Firmware completes the 4-step MCU SysEx handshake with Logic Pro (Device Query → Host Connection Query → Host Connection Reply → Confirmation) within 300ms
- [x] **MCU-02**: Logic Pro recognizes the controller as a Mackie Control Universal surface and begins sending bidirectional MIDI feedback
- [x] **MCU-03**: Channel strip buttons (REC, SOLO, MUTE, SELECT) send MCU Note Bang messages on MIDI channel 1 using note numbers 0–31
- [x] **MCU-04**: Transport buttons (Rewind, FF, Stop, Play, Record) send MCU Note Bang messages using note numbers 91–95
- [x] **MCU-05**: Sliders (faders) send 14-bit pitch bend on MIDI channels 1–8, not CC messages
- [x] **MCU-06**: Knobs send CC 16–23 (VPot encoder messages) per MCU protocol

### LED State Feedback

- [x] **LED-01**: Channel strip REC/SOLO/MUTE/SELECT LEDs update in response to Note On/Off messages from Logic (velocity 127 = on, 1 = blink, 0 = off)
- [x] **LED-02**: Transport state LEDs (Play, Record) reflect real Logic Pro state via incoming Note On/Off
- [ ] **LED-03**: Loop active, punch in/out, and metronome toggle states reflected in grid button LEDs
- [x] **LED-04**: ButtonRegistry extended with Note-to-Button lookup alongside existing CC-to-Button map

### Pickup Mode

- [x] **PICK-01**: Each fader tracks last known DAW value received via incoming pitch bend on MIDI channels 1–8
- [x] **PICK-02**: Fader MIDI output is suppressed until physical position crosses through (or within tolerance of) the DAW value
- [x] **PICK-03**: Channel button LED blinks (500ms period) while fader is out of sync with DAW value
- [x] **PICK-04**: LED blink stops and fader becomes active exactly when pickup occurs
- [x] **PICK-05**: All 8 faders re-enter pickup mode (blink) on every bank switch
- [x] **PICK-06**: Boundary edge case handled: faders at 0 or 127 pick up immediately if DAW value matches

### Grid Transport Layout

- [ ] **GRID-01**: Grid row 4 bottom: Shift (btn 13, LED), Record (btn 14, LED), Stop (btn 15, no LED), Play (btn 16, no LED) mapped to MCU transport note numbers
- [ ] **GRID-02**: Grid rows 1–3 (12 buttons): Bank Prev/Next, Loop Toggle, Punch In/Out, cursor navigation (Up/Down/Left/Right), Metronome/Click Toggle, Zoom mapped to MCU note numbers
- [ ] **GRID-03**: Shift button (btn 13) enables secondary functions when held (firmware-local state, not sent to DAW)
- [ ] **GRID-04**: Shift + button combinations send secondary MCU note messages for mode buttons

### Beat Chaser

- [ ] **BEAT-01**: A single LED sweeps across the 4×4 grid (14 LEDs with LEDs only) per beat, advancing one cell per beat
- [ ] **BEAT-02**: Beat timing derived from MIDI clock: counter increments on each of Logic's 24 PPQN clock pulses; one LED advances every 24 pulses
- [ ] **BEAT-03**: Beat chaser starts from cell 0 on MIDI Start (0xFA) message
- [ ] **BEAT-04**: Beat chaser resumes without resetting on MIDI Continue (0xFB) message
- [ ] **BEAT-05**: Beat chaser pauses on MIDI Stop (0xFC) message
- [ ] **BEAT-06**: Beat chaser never overwrites transport-owned LEDs (Play, Record, Stop, Shift)
- [ ] **BEAT-07**: All clock and animation logic is non-blocking (no delay()); uses millis() state machines

### Startup Animations

- [ ] **ANIM-01**: Compile-time config variable selects one of 4 startup animation styles
- [ ] **ANIM-02**: Cascade — LEDs sweep row by row, left to right, across the grid
- [ ] **ANIM-03**: Ripple — LEDs expand outward from center of grid
- [ ] **ANIM-04**: Sparkle — LEDs flicker in random order then settle
- [ ] **ANIM-05**: Flash — all LEDs blast on at once, then fade to correct state
- [ ] **ANIM-06**: After animation completes, LEDs settle to reflect current DAW state (not a fixed off state)
- [ ] **ANIM-07**: Startup animation is non-blocking — MIDI messages received during animation are processed

### Action Animations

- [ ] **ACT-01**: Record start: Record button LED pulses (blink pattern distinct from pickup blink)
- [ ] **ACT-02**: Bank switch: brief cascade sweep across 8 channel buttons (left to right)
- [ ] **ACT-03**: Shift held: subtle dimmed glow on non-active, non-transport LEDs
- [ ] **ACT-04**: All action animations are non-blocking; don't interrupt MIDI processing

### Power & Boot

- [x] **PWR-01**: A compile-time constant `LED_MAX_BRIGHTNESS` (default 180, range 0–255) in `src/pinDefines.h` caps the SoftPWM value used for all "LED on" states; `MCUButton::setLedState(127)` sets brightness to `LED_MAX_BRIGHTNESS` (not 255), ensuring the 24-LED array stays within USB 500mA power budget (Teensy ~150mA + 24 LEDs at ≤14mA avg = ~490mA max)
- [x] **BOOT-01**: On power-on, a cascade startup animation lights each LED in sequence (K1→K16 grid row by row, then P1→P8 track buttons) and turns all off before MIDI callback registration; animation is a blocking call in `setup()` before `usbMIDI` handlers are registered, completing within 2 seconds

### Codebase Cleanup (Prerequisites)

- [x] **FIX-01**: Fix copy-by-value Button bug in `inputManager.cpp:25` — use pointer or reference so DAW-driven `setLedState()` calls actually update physical LEDs
- [x] **FIX-02**: Remove `Serial.println("CONTROL CHANGE")` from MIDI callbacks — debug output causes timing jitter at 24 PPQN clock rates

## v2 Requirements

### Configuration

- **CONF-01**: Startup animation style selectable at runtime via button combo (no recompile needed)
- **CONF-02**: MIDI channel configurable for multi-surface Logic Pro setups

### Extended Grid Mappings

- **EXT-01**: MIDI channel 2–16 support for multi-surface configurations
- **EXT-02**: Custom CC mapping mode alongside MCU protocol (legacy compatibility)

### VPot Ring Display

- **VPT-01**: Parse incoming VPot ring CC (48–55) from Logic and store per-channel state for future display use

## Out of Scope

| Feature | Reason |
|---------|--------|
| Motorized faders | Hardware limitation — Teensy 3.5 PCB has no motor driver |
| RGB / multi-color LEDs | All LEDs are single color; no color hardware available |
| DIN MIDI | USB MIDI only — no DIN connector on PCB |
| EEPROM persistence | All state volatile; resets on power cycle (explicitly out of scope in PROJECT.md) |
| Ableton Live / other DAWs | Logic Pro MCU only for this milestone |
| VPot LED rings | This hardware has no physical VPot ring LEDs |
| Unit tests / test framework | No test infrastructure in place; validation is manual + hardware-in-loop |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| FIX-01 | Phase 1 | Pending |
| FIX-02 | Phase 1 | Pending |
| MCU-01 | Phase 1 | Complete |
| MCU-02 | Phase 1 | Complete |
| MCU-03 | Phase 1 | Complete |
| MCU-04 | Phase 1 | Complete |
| MCU-05 | Phase 1 | Complete |
| MCU-06 | Phase 1 | Complete |
| LED-01 | Phase 1 | Complete |
| LED-02 | Phase 1 | Complete |
| LED-03 | Phase 2 | Pending |
| LED-04 | Phase 1 | Complete |
| PWR-01 | Phase 1 | Complete |
| BOOT-01 | Phase 1 | Complete |
| PICK-01 | Phase 2 | Complete |
| PICK-02 | Phase 2 | Complete |
| PICK-03 | Phase 2 | Complete |
| PICK-04 | Phase 2 | Complete |
| PICK-05 | Phase 2 | Complete |
| PICK-06 | Phase 2 | Complete |
| GRID-01 | Phase 3 | Pending |
| GRID-02 | Phase 3 | Pending |
| GRID-03 | Phase 3 | Pending |
| GRID-04 | Phase 3 | Pending |
| BEAT-01 | Phase 4 | Pending |
| BEAT-02 | Phase 4 | Pending |
| BEAT-03 | Phase 4 | Pending |
| BEAT-04 | Phase 4 | Pending |
| BEAT-05 | Phase 4 | Pending |
| BEAT-06 | Phase 4 | Pending |
| BEAT-07 | Phase 4 | Pending |
| ANIM-01 | Phase 4 | Pending |
| ANIM-02 | Phase 4 | Pending |
| ANIM-03 | Phase 4 | Pending |
| ANIM-04 | Phase 4 | Pending |
| ANIM-05 | Phase 4 | Pending |
| ANIM-06 | Phase 4 | Pending |
| ANIM-07 | Phase 4 | Pending |
| ACT-01 | Phase 4 | Pending |
| ACT-02 | Phase 4 | Pending |
| ACT-03 | Phase 4 | Pending |
| ACT-04 | Phase 4 | Pending |

**Coverage:**
- v1 requirements: 42 total
- Mapped to phases: 42
- Unmapped: 0 ✓

---
*Requirements defined: 2026-02-21*
*Last updated: 2026-02-21 — added PWR-01 (USB current budget) and BOOT-01 (startup animation) to Phase 1*
