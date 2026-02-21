# Feature Research

**Domain:** Logic Pro / Mackie Control Universal hardware MIDI controller (non-motorized, monochromatic LEDs)
**Researched:** 2026-02-21
**Confidence:** MEDIUM-HIGH (MCU protocol note numbers from libMackieControl verified against multiple sources; Logic Pro behaviors confirmed via official Apple documentation; LED blink patterns confirmed via protocol spec; pickup mode UX confirmed via community forums and hardware examples)

---

## MCU Protocol Reference (Authoritative)

Source: libMackieControl MackieControl.md (verified against MidiBox protocol_mappings and TouchMCU documentation)

### Note Number Map

| Note | Button | Direction |
|------|--------|-----------|
| 0–7 | REC/RDY channels 1–8 | Bidirectional (LED + press) |
| 8–15 | SOLO channels 1–8 | Bidirectional |
| 16–23 | MUTE channels 1–8 | Bidirectional |
| 24–31 | SELECT channels 1–8 | Bidirectional |
| 32–39 | V-Select (knob press) channels 1–8 | Device → DAW |
| 40 | TRACK assignment | Bidirectional |
| 41 | SEND assignment | Bidirectional |
| 42 | PAN/SURROUND assignment | Bidirectional |
| 43 | PLUG-IN assignment | Bidirectional |
| 44 | EQ assignment | Bidirectional |
| 45 | INSTRUMENT assignment | Bidirectional |
| 46 | BANK LEFT | Device → DAW |
| 47 | BANK RIGHT | Device → DAW |
| 48 | CHANNEL LEFT | Device → DAW |
| 49 | CHANNEL RIGHT | Device → DAW |
| 50 | FLIP | Bidirectional |
| 51 | GLOBAL VIEW | Bidirectional |
| 69–75 | Mode buttons: MARKER, NUDGE, CYCLE, DROP, REPLACE, CLICK, SOLO | Bidirectional |
| 91 | REWIND | Bidirectional |
| 92 | FAST FWD | Bidirectional |
| 93 | STOP | Bidirectional |
| 94 | PLAY | Bidirectional |
| 95 | RECORD | Bidirectional |
| 96 | Cursor UP | Device → DAW |
| 97 | Cursor DOWN | Device → DAW |
| 98 | Cursor LEFT | Device → DAW |
| 99 | Cursor RIGHT | Device → DAW |
| 100 | ZOOM | Device → DAW |
| 101 | SCRUB | Bidirectional |
| 104–111 | Fader Touch channels 1–8 | Device → DAW |
| 112 | Fader Touch master | Device → DAW |

### LED Velocity Protocol

| Velocity | LED State |
|----------|-----------|
| 0 (0x00) | LED off |
| 1 (0x01) | LED blinking (any odd value 1–126) |
| 127 (0x7F) | LED solid on |

All button/LED communication uses **Note Bang**: Note On immediately followed by Note Off. Velocity encodes state.

### Fader Protocol

- Faders use **Pitch Bend** messages (0xE0–0xE8)
- MIDI channels 1–8 = fader channels 1–8
- MIDI channel 9 = master fader
- 14-bit resolution (0 = bottom, 16383 = top)

### Knob (V-Pot) Protocol

- **Rotation:** CC 16–23 (channels 1–8). Values 0–62 = clockwise, others = counterclockwise (relative, not absolute)
- **LED Ring:** CC 48–55, with mode encoding: 0–15 = single dot, 16–31 = boost/cut, 32–47 = wrap, 48–63 = spread

---

## Feature Landscape

### Table Stakes (Users Expect These)

These are non-negotiable for the controller to feel like a real MCU device in Logic Pro. Missing any = controller feels broken or incomplete.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| MCU protocol handshake / device ID | Logic Pro won't recognize device without proper SysEx identity response | MEDIUM | Logic sends inquiry, device must respond with MCU device ID; without this, Logic treats it as generic MIDI |
| Channel strip REC ARM (notes 0–7) | Core recording workflow — every MCU device has this | LOW | LED on = armed (velocity 127 from Logic), press sends Note Bang back |
| Channel strip MUTE (notes 16–23) | Core mixing workflow — expected on any channel strip controller | LOW | LED on = muted; Logic sends LED update; pressing button toggles and sends Note Bang |
| Channel strip SOLO (notes 8–15) | Core mixing workflow | LOW | LED on = soloed; "Rude Solo" global state implicit from any solo being active |
| Channel strip SELECT (notes 24–31) | Enables parameter editing for selected channel | LOW | LED on = selected; only one channel selected at a time typically |
| Transport: PLAY (note 94) | Fundamental — a controller without play is useless | LOW | LED on = playing (Logic sends LED update); button sends Note Bang |
| Transport: STOP (note 93) | Fundamental | LOW | No LED on standard stop button (no LED in hardware for this project's stop button) |
| Transport: RECORD (note 95) | Core recording — must reflect Logic's recording state | LOW | LED on = recording active; Logic Pro sends LED on when record starts |
| Bank Left/Right (notes 46–47) | With 8 strips and unlimited Logic tracks, banking is mandatory | MEDIUM | No LEDs on bank buttons; sends Note Bang; Logic handles channel strip re-assignment |
| Pickup mode for non-motorized faders | Without this, bank switches cause value jumps destroying the mix | HIGH | Must track DAW value via pitch bend feedback, compare to physical ADC, suppress output until crossing occurs |
| Channel button LED blink when out of pickup sync | Users need to know which faders are disconnected from DAW | MEDIUM | Uses MCU LED blink (velocity 1) on REC ARM LED of that strip OR on the channel button; blink stops on pickup |
| CYCLE / loop toggle (note ~71) | Loop is used constantly in production; LED must reflect active state | LOW | LED on = cycle active (Logic sends); pressing toggles; Logic manages loop boundaries |
| Transport: REWIND (note 91) | Navigation essential for DAW workflow | LOW | No LED required; sends Note Bang |
| Transport: FAST FWD (note 92) | Navigation essential | LOW | No LED required; sends Note Bang |
| CLICK / metronome toggle (note ~74) | Very common during tracking; LED must show if click is on | LOW | LED on = metronome active (Logic sends) |
| Cursor navigation (notes 96–99) | Used for parameter navigation, menu control | LOW | No LEDs; fire Note Bang on press |
| MIDI clock receipt for beat tracking | Beat chaser animation requires 24ppq clock from Logic | MEDIUM | handleClock() stub already exists; needs pulse counting logic |

### Differentiators (Competitive Advantage)

Features that make this controller feel polished and alive — beyond baseline MCU compliance.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| Beat chaser animation | Controller pulses with the music — visceral, immediate tempo feedback without a display | HIGH | Count 24 MIDI clock pulses per beat; advance LED position across 4×4 grid; only during playback (handleStart/Stop gate it) |
| 4 selectable startup animations | Personalization; makes the controller feel crafted not generic | MEDIUM | Cascade (row sweep), Ripple (center-out), Sparkle (random settle), Flash (all-on fade) — compile-time config; settle to DAW state after completion |
| Record blink animation | Recording is a high-stakes state; pulsing the RECORD LED makes it unmistakable | LOW | RECORD LED (note 95, velocity 1 = blink from MCU protocol) — Logic Pro itself may send blink already; verify and optionally reinforce locally |
| Bank switch animation | Cascade sweep across 8 channel buttons on bank change acknowledges the switch visually | MEDIUM | Triggered locally by bank button press; quick sequential LED flash across notes 24–31 (SELECT LEDs or similar) |
| Shift-held dimmed glow | Communicates shift is active without requiring a label; prevents accidental double-presses | LOW | When SHIFT held: reduce SoftPWM on all non-active LEDs to ~25% brightness; restore on release |
| Punch in/out on grid (note ~72 DROP) | Quick punch access from transport hub without reaching across the surface | LOW | Map grid button to MCU DROP note; LED reflects autopunch state from Logic |
| Zoom on grid (note 100) | Keeps navigation centralized on grid | LOW | Map grid button to ZOOM note; no LED feedback |
| Shift layer: REWIND/FF as marker prev/next | MCU already supports this via Marker mode; a shift layer makes it always accessible | MEDIUM | Hold SHIFT + REWIND/FF sends Marker mode navigation messages |
| LED fade on button release | SoftPWM already supports this; a brief dim-and-off on button release feels premium | LOW | Existing SoftPWM fade capability; applies to momentary-action buttons |

### Anti-Features (Commonly Requested, Often Problematic)

Features to explicitly NOT build in this milestone.

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| V-Pot LED ring emulation via SoftPWM | Rotary encoder with LED ring feedback looks professional | Hardware doesn't have ring LEDs; single LED per knob cannot represent ring modes (single-dot, boost/cut, spread) | Skip entirely; no knob LEDs in current hardware |
| Motorized fader position sync | Eliminates pickup mode UX friction | Hardware limitation — Teensy 3.5 + hardware has no motors | Pickup mode with blink feedback is the solution |
| SCRIBBLE STRIP / LCD display | MCU protocol includes 2×55 character LCD for track names | No display hardware; protocol SysEx for LCD is complex to generate | Send empty/default SysEx response or simply omit; Logic degrades gracefully |
| HUI protocol support | HUI is supported by Pro Tools and some other DAWs | HUI is a different protocol from MCU; supporting both doubles protocol complexity with no benefit since target DAW is Logic Pro only | MCU only |
| Per-bank LED state caching | Resuming LED state when banking back to prior channel set | Requires tracking 16+ LED states per possible bank, complex sync logic with no guarantee of accuracy | Trust Logic Pro to re-send LED state after bank switch — MCU protocol specifies DAW resends on bank change |
| Ableton Live / custom CC mapping | Backward compatibility with existing CC-based mapping (102–117, 20–27) | Two protocols simultaneously is confusing and creates message conflicts; compile-time selection already creates forks | Remove legacy CC mapping entirely; this firmware is Logic Pro / MCU only |
| EEPROM state persistence | Save LED or animation preference across power cycles | Out of scope per PROJECT.md; adds write-wear risk on Teensy | Use compile-time config variables for animation style |
| SysEx time code display mirroring | Echo time code to 7-segment if added later | No display hardware | Ignore incoming time code SysEx |

---

## Feature Dependencies

```
[MCU Protocol Handshake]
    └──required for──> [All LED feedback from Logic Pro]
                           └──required for──> [Rec arm LED]
                           └──required for──> [Mute LED]
                           └──required for──> [Solo LED]
                           └──required for──> [Select LED]
                           └──required for──> [Cycle LED]
                           └──required for──> [Record LED]
                           └──required for──> [Click LED]
                           └──required for──> [Drop / Punch LED]

[Fader Pitch Bend receipt (DAW → device)]
    └──required for──> [Pickup mode sync tracking]
                           └──drives──> [Channel button blink when out of sync]
                           └──gates──> [Fader output suppression until crossing]

[MIDI Clock receipt (handleClock)]
    └──required for──> [Beat chaser animation]
                           └──gated by──> [handleStart / handleStop state]

[Shift button held state]
    └──enables──> [Shift layer secondary functions]
    └──enables──> [Dimmed glow on non-active LEDs]

[Bank switch (BANK LEFT/RIGHT note 46–47)]
    └──triggers──> [Bank switch cascade animation]
    └──invalidates──> [Pickup mode sync state for all 8 channels]
                          (all faders must re-enter pickup mode after bank switch)

[Startup animation]
    └──must complete before──> [Initial LED state sync from Logic Pro]
```

### Dependency Notes

- **MCU handshake required for all LED feedback:** Logic Pro will not send any LED state updates until the device identifies itself with the MCU SysEx identity response. This is the single highest-priority prerequisite.
- **Pitch bend receipt required for pickup mode:** The device must process incoming pitch bend messages (Logic → device fader position feedback) to know the DAW's current value and detect when the physical pot crosses it.
- **Bank switch invalidates pickup sync:** When the user banks to a different channel group, all 8 faders are now controlling different DAW channels. Any stored DAW-side values are now stale. All faders must re-enter "out of sync" state (LED blink) until each is physically crossed.
- **Beat chaser gated by transport:** Beat chaser only runs during playback. handleStart() enables counting; handleStop() resets animation and turns off the chaser LED.
- **Startup animation must settle:** After any startup animation, the firmware must query or wait for Logic to re-send LED state. Since MCU doesn't have a "request state" command, the standard approach is to let Logic resend upon its next state update; startup animation should end with all LEDs OFF so any Logic-sent state appears cleanly.

---

## MVP Definition

### Launch With (v1 — MCU-compliant baseline)

- [ ] MCU SysEx identity handshake — required for Logic Pro recognition
- [ ] Channel strip REC/SOLO/MUTE/SELECT (notes 0–31) bidirectional — core mixer control
- [ ] Transport: PLAY, STOP, RECORD (notes 94, 93, 95) with LED feedback — essential recording workflow
- [ ] BANK LEFT/RIGHT (notes 46–47) — mandatory for >8 track projects
- [ ] CYCLE/loop toggle (note ~71) with LED — heavily used production feature
- [ ] CLICK/metronome toggle (note ~74) with LED — tracking necessity
- [ ] Pickup mode with blink feedback — non-motorized UX requirement; without this the controller is unusable after bank switches
- [ ] Grid transport hub layout (rows 1–3 for nav/mode, row 4 = Shift + Record + Stop + Play)
- [ ] Cursor navigation (notes 96–99) — basic menu/parameter navigation

### Add After Validation (v1.x)

- [ ] Beat chaser animation — high delight value, low user-facing risk; add once transport callbacks are stable
- [ ] Startup animations (all 4 styles) — polish feature; add after core protocol is proven
- [ ] Bank switch cascade animation — visual nicety; depends on bank switch being stable
- [ ] Record blink animation — confirm whether Logic Pro already sends velocity 1 (blink) on record; if yes, it's free

### Future Consideration (v2+)

- [ ] REWIND/FF transport and Marker mode navigation — useful but not essential for initial validation
- [ ] PUNCH/DROP button (autopunch) — niche workflow; add when core is stable
- [ ] ZOOM button on grid — low-priority navigation aid
- [ ] Full shift layer secondary functions — scope after shift button itself is working

---

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| MCU handshake | HIGH | MEDIUM | P1 |
| REC/SOLO/MUTE/SELECT LEDs | HIGH | LOW | P1 |
| Transport PLAY/STOP/RECORD | HIGH | LOW | P1 |
| Bank Left/Right | HIGH | MEDIUM | P1 |
| Pickup mode + blink feedback | HIGH | HIGH | P1 |
| CYCLE toggle + LED | HIGH | LOW | P1 |
| CLICK toggle + LED | MEDIUM | LOW | P1 |
| Cursor navigation | MEDIUM | LOW | P1 |
| Beat chaser animation | HIGH | MEDIUM | P2 |
| Startup animations | MEDIUM | MEDIUM | P2 |
| Shift layer (held state + dim glow) | MEDIUM | LOW | P2 |
| Bank switch animation | LOW | MEDIUM | P2 |
| Record pulse animation | MEDIUM | LOW | P2 |
| REWIND/FF navigation | MEDIUM | LOW | P2 |
| DROP/punch toggle | LOW | LOW | P3 |
| ZOOM on grid | LOW | LOW | P3 |
| Full shift layer secondaries | LOW | HIGH | P3 |

**Priority key:**
- P1: Must have for launch
- P2: Should have, add when possible
- P3: Nice to have, future consideration

---

## Logic Pro LED State Mapping (What Logic Sends)

This is what Logic Pro communicates via MCU protocol — the meaning of each LED state this hardware must display correctly.

| LED / Note | Logic State When ON (velocity 127) | Logic State When BLINKING (velocity 1) | Logic State When OFF (velocity 0) |
|------------|-----------------------------------|-----------------------------------------|-----------------------------------|
| REC note 0–7 | Channel is armed for recording | (rare — may indicate monitoring state) | Channel not armed |
| SOLO note 8–15 | Channel is soloed | — | Channel not soloed |
| MUTE note 16–23 | Channel is muted | — | Channel not muted |
| SELECT note 24–31 | Channel is selected for editing | — | Channel not selected |
| RECORD note 95 | Recording is in progress | Recording pending / pre-count | Not recording |
| PLAY note 94 | Playback active | — | Stopped/paused |
| CYCLE note ~71 | Cycle/loop mode active | — | Cycle mode off |
| DROP note ~72 | Autopunch mode active | — | Autopunch off |
| REPLACE note ~73 | Replace mode active | — | Replace mode off |
| CLICK note ~74 | Metronome is on | — | Metronome off |
| SOLO (mode) note ~75 | Solo lock active | — | Solo lock off |

**Source:** Apple Logic Pro Control Surfaces Support documentation (multiple pages); MidiBox protocol mappings; libMackieControl specification

---

## Logic Pro Specific Behaviors (Not Generic MCU)

Confidence: MEDIUM (from Apple official docs, Logic Pro 9 and current Logic Pro)

1. **Auto-detect:** Logic Pro auto-detects Mackie Control devices on connection. No manual setup required IF the device responds correctly to the MCU SysEx inquiry.

2. **Bank behavior:** Logic remembers the last bank position per view type (audio, instruments, busses). Banking back to audio tracks returns to your last audio channel bank, not track 1.

3. **OPTION modifier on SELECT:** OPTION+SELECT creates a new track in Logic (not just selects). This is Logic-specific behavior that differs from other DAWs.

4. **SHIFT+PLAY = Pause:** Logic supports pause via SHIFT+PLAY — not a stop-and-return. Useful for drop-in monitoring.

5. **SHIFT+CLICK = External Sync + MMC:** Enabling external sync and MIDI Machine Control simultaneously via a single modifier combo.

6. **RUDE SOLO behavior:** Logic lights the Rude Solo LED (a separate LED on physical MCU hardware right of the display) when ANY channel is soloed. This controller has no dedicated Rude Solo LED — can be communicated via a grid button LED instead if desired.

7. **Assignment buttons change V-Pot behavior:** Pressing TRACK, PAN, SEND, EQ, PLUG-IN, or INSTRUMENT changes what all 8 V-Pots control. LED on assignment button shows which mode is active. Logic resends V-Pot CC display data for the new mode.

8. **F-key assignments:** F1–F8 recall screensets 1–7 and close floating windows by default. SHIFT+F1–F8 opens editors (Tracks, Mixer, Event Editor, Score, Step, Piano Roll, Control bar, Project Audio Browser).

---

## Competitor Feature Analysis

Reference controllers: Mackie MCU Pro, PreSonus FaderPort 8, Icon QCon Pro G2, Behringer X-Touch

| Feature | Mackie MCU Pro | FaderPort 8 | Icon QCon Pro G2 | This Controller |
|---------|---------------|-------------|-----------------|----------------|
| Channel count | 8 | 8 | 8 | 8 |
| Motorized faders | YES | YES | YES | NO — pickup mode |
| Scribble strips (LCD per channel) | YES | YES | YES | NO — no display |
| Transport section | Full | Full | Full | Grid-based hub |
| Beat/meter display | 7-segment SMPTE+BBT | 7-segment | 7-segment | LED grid chaser |
| Startup animation | None | Sweep | Sweep | 4 selectable styles |
| Shift layer | YES | YES | YES | YES |
| V-Pot LED rings | YES (3-color) | YES | YES | NO |
| Form factor | Desktop 8-channel | Desktop 8-channel | Desktop 8-channel | Custom hardware |

**Key differentiation:** No display, no motorized faders, no V-pot rings — this controller compensates with exceptional LED animation (beat chaser, startup styles, pickup blink) that commercial controllers don't have. The grid becomes the visual centerpiece that commercial controllers lack.

---

## Sources

- [Apple Logic Pro: Overview of Mackie Control](https://support.apple.com/guide/logicpro-css/mackie-control-overview-ctls7222820e/mac) — Official, current
- [Apple Logic Pro: Channel Strip Buttons and LEDs](https://support.apple.com/guide/logicpro-css/channel-strip-buttons-and-leds-ctls722264b2/mac) — Official, current
- [Apple Logic Pro: Transport Buttons](https://support.apple.com/guide/logicpro-css/transport-buttons-overview-ctls72228156/mac) — Official, current
- [Apple Logic Pro: Modifier Buttons](https://support.apple.com/guide/logicpro-css/modifier-buttons-ctls72228aa4/mac) — Official, current
- [Apple Logic Pro: Function Keys](https://support.apple.com/guide/logicpro-css/assign-function-keys-table-ctls72227e31/mac) — Official, current
- [Apple Logic Pro: Cycle Button](https://support.apple.com/guide/logicpro-css/cycle-button-ctls722248d8/mac) — Official, current
- [Apple Logic Pro: Bank Buttons](https://support.apple.com/guide/logicpro-css/mackie-control-bank-buttons-ctls72224042/mac) — Official, current
- [Apple Logic Pro: Replace, Click, Solo Buttons](https://support.apple.com/guide/logicpro-css/replace-click-and-solo-buttons-ctls72225754/mac) — Official, current
- [Apple Logic Pro 9: Channel Strip Controls](https://help.apple.com/logicpro/mac/9.1.6/en/logicpro/controlsurfacessupport/chapter_3_section_3.html) — Older but still accurate for MCU protocol
- [Apple Logic Pro 9: Transport Zone](https://help.apple.com/logicpro/mac/9.1.6/en/logicpro/controlsurfacessupport/chapter_3_section_11.html) — Older but protocol-level details still valid
- [libMackieControl: MackieControl.md](https://github.com/Do-sth-sharp/libMackieControl/blob/main/doc/MackieControl.md) — Protocol specification, note numbers, CC assignments (MEDIUM confidence — open source implementation, verified against MidiBox)
- [MidiBox: MCU Protocol Mappings](http://www.midibox.org/dokuwiki/doku.php?id=mc_protocol_mappings) — Transport and strip note assignments
- [TouchMCU: Mackie Control Protocol](https://github.com/NicoG60/TouchMCU/blob/main/doc/mackie_control_protocol.md) — LED velocity protocol (0=off, 1=blink, 127=on), pitch bend fader protocol
- [Arduino Forum: Fader Pickup/Catch Mode](https://forum.arduino.cc/t/fader-pickup-catch-mode/1206227) — Bidirectional crossing implementation, FSM approach
- [KVR Audio: Control Surface Fader Pickup Mode](https://www.kvraudio.com/forum/viewtopic.php?t=544413) — Community validation of pickup UX patterns

---
*Feature research for: Logic Pro / Mackie Control Universal hardware MIDI controller*
*Researched: 2026-02-21*
