# Project Research Summary

**Project:** Phaedr MIDI Controller — MCU Protocol + LED Animation Milestone
**Domain:** Teensy 3.5 USB MIDI controller firmware (Mackie Control Universal protocol)
**Researched:** 2026-02-21
**Confidence:** MEDIUM-HIGH

## Executive Summary

This milestone converts an existing CC-based MIDI controller into a Mackie Control Universal (MCU) compliant device for Logic Pro. The MCU protocol is not a library — it is a set of specific MIDI message conventions: buttons use Note On/Off (not CC), faders use Pitch Bend per dedicated MIDI channel (not CC on channel 1), and LED state is communicated via note velocity (0=off, 1=blink, 127=on). No new libraries are required; all message types are handled natively by the existing Teensyduino `usbMIDI` object. The stack stays entirely unchanged.

The most important architectural constraint is that everything depends on a single gate: the MCU SysEx handshake. Logic Pro will not send any LED state, fader position, or transport feedback until the controller responds correctly to a 4-step challenge-response exchange (device query, host connection query, host connection reply, confirmation). This must be implemented first and verified before any other MCU work begins. All remaining features — pickup mode, beat chaser, animations — have clear patterns and can be built incrementally on top of that foundation.

The primary risks are protocol-level: the SysEx handshake must be exactly right, faders must send Pitch Bend (not CC) on per-channel MIDI channels, and the MIDI Continue message must be handled separately from Start or the beat chaser will drift on mid-song playback. A secondary risk is the existing copy-by-value Button bug in InputManager, which will silently break all LED feedback from Logic and must be fixed before MCU work begins. All five critical pitfalls have known, concrete prevention strategies.

---

## Key Findings

### Recommended Stack

The existing stack requires no changes. `usbMIDI` (Teensyduino built-in), `Bounce2`, and `SoftPWM` cover all MCU functionality. The build flag `-DUSB_MIDI_SERIAL` is already set. The Control-Surface library (tttapa) was evaluated and rejected: it requires adopting its entire MIDI interface architecture and would force a full rewrite of all existing Button, Potentiometer, InputManager, and ButtonRegistry code. Implementing MCU messages directly through `usbMIDI` is the correct approach for this codebase.

**Core technologies:**
- `usbMIDI` (Teensyduino built-in): All MIDI I/O including SysEx, Pitch Bend, Note, CC, and RealTime — no new library needed
- `Bounce2` ^2.70: Button debouncing — unchanged
- `SoftPWM` ^1.0.1: LED brightness and blink — unchanged; uses PIT/IntervalTimer, does not conflict with usbMIDI callbacks
- `elapsedMillis` (Teensy built-in): Non-blocking animation timing — already available, no installation required
- `Arduino framework` via PlatformIO: Unchanged; `setup()`/`loop()` pattern is fully compatible with MCU additions

**Critical API note:** Teensyduino `sendPitchBend` uses -8192 to +8191 (centered at 0). MCU spec uses 0–16383. All fader math must offset by 8192. The mapping is: `teensyValue = (adcValue * 16383 / 1023) - 8192`.

### Expected Features

**Must have (table stakes — P1):**
- MCU SysEx handshake — Logic Pro will not send LED feedback without this; it is the prerequisite for every other feature
- Channel strip REC ARM (notes 0–7), SOLO (8–15), MUTE (16–23), SELECT (24–31) — bidirectional with LED feedback via velocity protocol
- Transport: PLAY (note 94), STOP (note 93), RECORD (note 95) — with LED feedback from Logic
- BANK LEFT/RIGHT (notes 46–47) — mandatory for projects with more than 8 tracks
- Pickup mode for non-motorized faders — suppresses output until physical crosses DAW value; without this, bank switches destroy mix levels
- Channel button LED blink (velocity 1) while fader is out of pickup sync
- CYCLE/loop toggle (note ~71) and CLICK/metronome (note ~74) with LED feedback
- Cursor navigation (notes 96–99)
- Grid transport hub layout (bottom row: Shift + Record + Stop + Play)

**Should have (differentiators — P2):**
- Beat chaser animation — grid LED advances one position per beat using 24 PPQN MIDI clock; visceral tempo feedback without a display
- Startup animations: CASCADE, RIPPLE, SPARKLE, FLASH — selected at compile time via `#define`
- Bank switch cascade animation — brief sequential LED sweep across channel buttons on bank change
- Record blink / pulse animation — RECORD LED pulses during active recording (Logic may send velocity 1 natively; verify before implementing locally)
- Shift-held dimmed glow — non-active LEDs dim to ~25% while Shift is held

**Defer to v2+:**
- REWIND/FF transport buttons and Marker mode navigation
- PUNCH/DROP autopunch button
- ZOOM button on grid
- Full shift layer secondary functions (marker prev/next, etc.)

**Do not build (anti-features):**
- VPot LED ring emulation (no ring hardware exists)
- Motorized fader sync (no motors)
- SCRIBBLE STRIP / LCD display (no display hardware)
- HUI protocol support (target DAW is Logic Pro only)
- Per-bank LED state caching (trust Logic to resend on bank change)
- Ableton Live / legacy CC mapping alongside MCU (causes routing conflicts; remove CC mapping entirely)

### Architecture Approach

Three new modules layer onto the existing codebase without replacing it. `MCULayer` owns the SysEx handshake, MCU note-to-semantic-action mapping, pitch bend fader routing, and all usbMIDI send calls for outbound protocol messages. `AnimationManager` owns all time-driven animation state using `elapsedMillis` timers, polling `ClockCounter` for beat ticks. `ClockCounter` is a minimal 24-PPQN counter wired to `handleClock()`, `handleStart()`, `handleStop()`, and the new `handleContinue()`. Existing classes — `Button`, `Potentiometer`, `InputManager`, `ButtonRegistry` — are extended, not replaced.

**Major components:**
1. `MCULayer` (new) — SysEx handshake state machine, MCU protocol message dispatch, VPot CC parsing, fader pitch bend routing
2. `AnimationManager` (new) — startup sequences, beat chaser, action animations, shift glow; uses `elapsedMillis`; calls `SoftPWMSet()` directly on LED pins during animations
3. `ClockCounter` (new) — 24 PPQN pulse counter; exposes `beatElapsed()` flag; handles Start/Continue/Stop correctly
4. `ButtonRegistry` (extended) — add `noteNumToButton: std::map<int, Button*>` alongside existing CC map
5. `Button` (extended) — add `setLedBlink()` using `elapsedMillis` + SoftPWM; add `getLEDPin()` for AnimationManager
6. `Potentiometer` (extended) — add `PickupState` enum, `_dawValue`, `crossedThrough()`, `isWaiting()` per instance
7. `InputManager` (extended) — poll `isWaiting()` per pot each loop to drive blink on paired channel button; send MCU Note messages instead of CC

**Key patterns:**
- Animations: `elapsedMillis` state machine in `AnimationManager::tick()`, called every `loop()`. Never `delay()`.
- Clock callbacks: increment counter only; no LED work inside callback
- Animation vs DAW LED ownership: AnimationManager writes directly to `SoftPWMSet()` during animations; ButtonRegistry owns LEDs during IDLE. Clean handoff on state transition.
- Pickup mode: per-instance `PickupState` on each `Potentiometer`; faders re-enter ACTIVE independently when each crosses its individual DAW target

### Critical Pitfalls

1. **Incomplete SysEx handshake** — Logic shows the surface but sends zero feedback. Implement the full 4-step challenge-response using `setHandleSystemExclusive`. Respond to Device Query (`14 00`) and send Confirmation (`14 03`) after verifying the Host Connection Reply (`14 02`). This is the single Phase 1 gate: verify with MIDI monitor before proceeding.

2. **Faders sending CC instead of Pitch Bend** — Logic ignores CC on channel 1 for fader control entirely. Extend `Potentiometer` (or create a `Fader` subclass) to call `usbMIDI.sendPitchBend(value, channel)` on the correct per-fader MIDI channel. Do not reuse the existing CC send path.

3. **Pickup mode boundary lockup** — When both physical and DAW values are at 0 or both at 127, a pure crossing check never fires. Add a tolerance-based check alongside crossing detection: if `abs(physical - dawValue) <= 3`, pick up immediately. Also check `atSameExtreme` (both near 0 or both near max). Unit-testable without hardware.

4. **Missing `handleContinue` registration** — Logic sends `Continue` (0xFB) on mid-song resume and cycle loop restarts — not `Start` (0xFA). Without `setHandleContinue`, the clock counter resets to beat 1 on every resume, making the beat chaser jump. Register `handleContinue` and do NOT reset the clock counter in it; only `Start` resets.

5. **Copy-by-value Button bug in InputManager (pre-existing)** — Buttons stored by value rather than pointer means `setLedState()` writes to a temporary, silently doing nothing. Fix this before adding any MCU features or all LED feedback from Logic will appear broken.

---

## Implications for Roadmap

Based on research, the dependency chain is explicit: SysEx handshake unlocks all LED feedback; LED feedback is required to verify channel strip and transport work; pickup mode requires fader pitch bend receipt which requires MCU registration; beat chaser requires `handleContinue` alongside `handleClock`. This maps directly to a 4-phase build.

### Phase 1: MCU Protocol Foundation

**Rationale:** Logic Pro sends zero useful feedback until the SysEx handshake completes. Every other feature depends on this working. This phase establishes the entire inbound/outbound MCU message infrastructure.

**Delivers:** Logic Pro recognizes the device as Mackie Control; LED state for all channel strip buttons (REC/SOLO/MUTE/SELECT) and transport buttons updates correctly from Logic; faders send Pitch Bend and Logic fader strips respond.

**Addresses:**
- MCU SysEx identity handshake
- Channel strip REC ARM, SOLO, MUTE, SELECT (notes 0–31) bidirectional
- Transport PLAY, STOP, RECORD (notes 93–95) with LED feedback
- Fader Pitch Bend send (replacing CC)

**Avoids:**
- Pitfall 1 (SysEx handshake): Implement full challenge-response, verify with MIDI monitor
- Pitfall 2 (fader CC vs Pitch Bend): Switch faders to sendPitchBend on per-channel channels
- Copy-by-value Button bug: Fix before this phase begins

**Pre-phase prerequisite:** Fix the copy-by-value Button bug in InputManager before writing any MCU feature code.

**Verifiable:** Logic Pro shows "Mackie Control" in Control Surfaces preference pane AND responds to button presses with LED updates.

### Phase 2: Pickup Mode

**Rationale:** Banking to different channel groups is mandatory for any project with more than 8 tracks. Without pickup mode, bank switches cause immediate fader value jumps that destroy mix levels. This phase makes the controller safe to use in a real session.

**Delivers:** Faders suppress output after bank switch until physical position crosses DAW value; paired channel button blinks (velocity 1) while out of sync; blink clears at exact crossover.

**Addresses:**
- Pickup mode for non-motorized faders
- Channel button LED blink feedback during desync
- BANK LEFT/RIGHT (notes 46–47)

**Avoids:**
- Pitfall 3 (VPot byte encoding): Also implement VPot CC decoder here (parse CC 48–55; hardware has no ring LEDs so store and ignore, but parsing must be correct)
- Pitfall 4 (pickup boundary lockup): Add tolerance check alongside crossing detection

**Verifiable:** Bank switch triggers blink on all 8 channel buttons; sweeping fader through DAW position clears blink exactly at crossover; fader at 0 with DAW at 0 responds immediately without requiring additional movement.

### Phase 3: Grid Layout and Transport Mapping

**Rationale:** With protocol foundation and fader behavior solid, wire the physical grid to the full MCU note map. This phase completes the controller's usable control surface — CYCLE, CLICK, BANK, cursor navigation, and the transport grid hub layout.

**Delivers:** All grid and track buttons send correct MCU note numbers; bottom grid row is transport hub (Shift, Record, Stop, Play); CYCLE and CLICK toggles with LED feedback; cursor navigation; shift layer state tracking.

**Addresses:**
- CYCLE/loop toggle (~note 71) with LED
- CLICK/metronome toggle (~note 74) with LED
- Cursor navigation (notes 96–99)
- Grid transport hub layout (bottom row)
- Shift button held state (single `bool _shiftHeld` in InputManager)
- REWIND, FAST FWD (notes 91–92) — no LED required

**Avoids:** Validate all grid note numbers against Logic Pro's specific MCU implementation — some mode button note numbers in the STACK.md reference are flagged LOW confidence and need hardware verification.

**Verifiable:** Play, Stop, Record, Cycle, Click all control Logic and receive LED feedback; cursor buttons navigate Logic menus.

### Phase 4: Animation Manager

**Rationale:** All functional MCU behavior is complete after Phase 3. Phase 4 adds the differentiating visual layer — the features that commercial controllers lack. These are safe to defer because they layer on top of stable transport and clock infrastructure.

**Delivers:** Startup animation (one of four selectable styles) on power-on; beat chaser grid LED advances with Logic playback tempo; bank switch sweep animation; record pulse animation; shift-held dimmed glow.

**Addresses:**
- Beat chaser animation (24 PPQN MIDI clock → grid LED position)
- Startup animations: CASCADE, RIPPLE, SPARKLE, FLASH (compile-time `#define`)
- Bank switch cascade animation
- Record pulse animation
- Shift-held dimmed glow on non-transport LEDs

**Avoids:**
- Pitfall 5 (missing handleContinue): Register `handleContinue` and do NOT reset clock counter; only `Start` resets
- Anti-pattern: heavy work in clock callback — set flag only, do LED work in `AnimationManager::tick()`
- Anti-pattern: beat chaser writing to transport LED cells (Record, Play) — those cells are protected

**Verifiable:** CASCADE animation visible at startup; beat chaser advances with BPM at correct phase when starting from bar 1; beat chaser does NOT reset on mid-song resume (Continue message); beat chaser stays in phase through loop cycle restarts.

### Phase Ordering Rationale

- Phase 1 before everything: The SysEx handshake is a hard gate. No LED feedback from Logic arrives without it. Building and testing anything else first produces only partial signal.
- Phase 2 before Phase 3: Pickup mode depends on fader pitch bend receipt (wired in Phase 1) but must be validated before the grid layout is complete, because bank switch behavior needs working faders to verify.
- Phase 3 before Phase 4: The `handleClock`, `handleStart`, `handleStop`, and `handleContinue` callbacks must be registered and tested before wiring the beat chaser to them. Phase 3 establishes the clean transport event model that Phase 4 consumes.
- BANK LEFT/RIGHT spans Phases 2 and 3: Bank note messages (46–47) are part of the grid layout (Phase 3) but bank switch invalidates pickup sync for all 8 faders (Phase 2). Implement the pickup invalidation logic in Phase 2 even if the physical bank buttons are not yet mapped.

### Research Flags

Phases likely needing verification during implementation:

- **Phase 1 (SysEx handshake):** The challenge-response algorithm and exact SysEx byte sequence are MEDIUM confidence (community documentation, no official Mackie spec). Verify with MIDI monitor that Logic sends a `14 00` Device Query and that the full 4-message sequence completes. If Logic does not send the query automatically, it may need to be triggered by removing and re-adding the control surface in Logic preferences.
- **Phase 3 (Grid note numbers):** Several mode button note assignments (~69–75 range for CYCLE, DROP, REPLACE, CLICK, SOLO) are flagged as needing Logic Pro verification. The STACK.md lists them, FEATURES.md notes some at "~note" with approximation. Test each button against a MIDI monitor with Logic Pro open before committing note assignments.
- **Phase 4 (Record LED blink):** Logic Pro may already send velocity 1 (blink) on the RECORD button note (95) during pre-count and pending states. Verify before implementing local blink animation — if Logic sends it natively, the MCU LED velocity protocol handles it for free.

Phases with well-documented patterns (research not needed):

- **Phase 2 (Pickup mode FSM):** The crossing-plus-tolerance algorithm is verified from multiple Arduino community implementations. The pattern is clear and the edge cases are documented.
- **Phase 4 (Beat chaser):** MIDI 24 PPQN is a formal standard. The counter pattern is trivial. The only validation needed is the handleContinue behavior, which is documented in Apple's official MIDI sync docs.

---

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | All tools already in use; no new dependencies; Teensyduino API verified from official PJRC docs |
| Features | MEDIUM-HIGH | MCU note table verified across 3 independent sources (libMackieControl, TouchMCU, tttapa Control-Surface); Logic Pro behaviors confirmed via Apple official documentation; some mode button note numbers need hardware verification |
| Architecture | HIGH | Component decomposition is clean and well-justified; all patterns verified against official Teensy docs and working community implementations; `elapsedMillis` and `crossedThrough` patterns are straightforward |
| Pitfalls | MEDIUM | SysEx handshake details are MEDIUM confidence (no official Mackie spec; community reverse-engineering); all other pitfalls have HIGH confidence prevention strategies from official sources |

**Overall confidence:** MEDIUM-HIGH

### Gaps to Address

- **SysEx handshake verification:** Must be validated against actual Logic Pro behavior in Phase 1. The exact byte sequence is well-documented from community sources, but Logic Pro's specific behavior (does it always send the Device Query? does it retry?) is not officially documented. Mitigation: test with MIDI monitor immediately after Phase 1 implementation and before any other work.
- **Mode button note numbers (notes 69–75 range):** The CYCLE (~71), DROP (~72), REPLACE (~73), CLICK (~74), SOLO-mode (~75) note assignments use approximate values in some sources. Use a MIDI monitor to capture what Logic actually sends and confirms note assignments before wiring them to physical buttons.
- **Teensyduino pitch bend API version:** STACK.md notes that pre-1.41 Teensyduino used 0–16383 while current uses -8192 to +8191. Confirm by checking the installed `usb_midi.h` header or testing a known value against the MIDI monitor before writing pickup mode math.
- **SoftPWM IntervalTimer stability:** PITFALLS.md flags a known Teensy 3.5 IntervalTimer crash after thousands of activations. SoftPWM uses one IntervalTimer. Keep total active IntervalTimers at 3 or fewer. Monitor for stability in extended sessions.

---

## Sources

### Primary (HIGH confidence)
- [PJRC Teensyduino USB MIDI documentation](https://www.pjrc.com/teensy/td_midi.html) — sendPitchBend, sendNoteOn, sendSysEx, setHandleClock, setHandlePitchChange API
- [PJRC cores/teensy3/usb_midi.h](https://github.com/PaulStoffregen/cores/blob/master/teensy3/usb_midi.h) — sendSysEx signature, USB_MIDI_SYSEX_MAX (290 bytes)
- [PJRC elapsedMillis timing](https://www.pjrc.com/teensy/td_timing_elaspedMillis.html) — non-blocking animation timer pattern
- [PJRC IntervalTimer documentation](https://www.pjrc.com/teensy/td_timing_IntervalTimer.html) — 4-timer limit on Teensy 3.x
- [PJRC SoftPWM library documentation](https://www.pjrc.com/teensy/td_libs_SoftPWM.html) — PIT/IntervalTimer usage, CPU overhead
- [tttapa/Control-Surface MCU note constants](https://tttapa.github.io/Control-Surface/Doxygen/de/d44/group__MCU__Notes.html) — verified note number table
- [Apple Logic Pro MIDI Sync documentation](https://support.apple.com/guide/logicpro/midi-synchronization-settings-lgcp72142361/10.7/mac/11.0) — Start vs Continue vs Song Position Pointer behavior
- [Apple Logic Pro Control Surfaces documentation](https://support.apple.com/guide/logicpro-css/) — channel strip LEDs, transport buttons, bank buttons, modifier behavior

### Secondary (MEDIUM confidence)
- [NicoG60/TouchMCU — mackie_control_protocol.md](https://github.com/NicoG60/TouchMCU/blob/main/doc/mackie_control_protocol.md) — SysEx handshake sequence, challenge-response algorithm, LED velocity protocol, VPot encoding
- [Do-sth-sharp/libMackieControl — MackieControl.md](https://github.com/Do-sth-sharp/libMackieControl/blob/main/doc/MackieControl.md) — full note number table, fader channel assignment, VPot CC format, challenge-response algorithm
- [PJRC Forum — Sonar/Cakewalk Mackie SysEx](https://forum.pjrc.com/threads/50036-Sonar-Cakewalk-Mackie-SysEx) — working MCU SysEx handshake on Teensy
- [PJRC Forum — Receive Pitch Bend data (Mackie HUI)](https://forum.pjrc.com/index.php?threads/receive-pitch-bend-data-mackie-hui-protocol.50005/) — fader receive pattern
- [Cycling '74 Forum — LogicControl initialization](https://cycling74.com/forums/logiccontrol-and-maxmsp-initialization) — 300ms handshake window
- [Arduino Forum — Fader pickup/catch mode](https://forum.arduino.cc/t/fader-pickup-catch-mode/1206227) — pickup mode FSM pattern
- [MidiBox: MCU Protocol Mappings](http://www.midibox.org/dokuwiki/doku.php?id=mc_protocol_mappings) — transport and strip note assignments cross-reference
- [Teensy Forum — IntervalTimer crash on 3.5/3.6](https://forum.pjrc.com/index.php?threads/teensy-3-6-3-5-intervaltimer-sketch-crashes-after-thousands-of-activations.56997/) — known stability issue

### Tertiary (LOW confidence — needs validation)
- [Zynthian Forum: MCU Device IDs](https://discourse.zynthian.org/t/mackie-control-as-supported-midi-controller/11717/15) — Device ID `0x14` for MCU Universal; consistent with other sources but needs verification against Logic Pro specifically

---

*Research completed: 2026-02-21*
*Ready for roadmap: yes*
