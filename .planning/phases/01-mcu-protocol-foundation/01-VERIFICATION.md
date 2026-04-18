---
phase: 01-mcu-protocol-foundation
verified: 2026-02-21T00:00:00Z
status: passed
score: 13/13 must-haves verified
re_verification: false
gaps: []
human_verification:
  - test: "Logic Pro surface recognition end-to-end"
    expected: "Logic Pro Control Surfaces panel shows 'Mackie Control' as an active recognized surface after Teensy powers on; SysEx handshake messages visible in MIDI monitor"
    why_human: "Cannot verify DAW recognition without running Logic Pro against live hardware; handshake state machine exists and is correct but DAW response cannot be simulated in code"
  - test: "Channel strip button LED feedback"
    expected: "Pressing a P1-P8 track button causes Logic to send Note On back (note 0-7) and the corresponding hardware LED lights up"
    why_human: "LED response path is fully wired in code but requires live DAW + hardware to observe the physical LED"
  - test: "Startup animation visual correctness"
    expected: "On power-on, a 5-LED pulse window sweeps K1->P8 and back twice (~4.4s) before any MIDI activity; all 24 LEDs participate including K13 and P8"
    why_human: "Animation code and SoftPWM registration verified correct; visual fidelity and hardware LED health require physical observation"
  - test: "Slider pitch bend to Logic fader"
    expected: "Moving a slider causes Logic's on-screen fader to move with full range; MIDI monitor confirms Pitch Bend (not CC) on fader's MIDI channel 1-8"
    why_human: "Fader::read() sends correct Pitch Bend with -8192 offset; visual confirmation in DAW requires hardware"
  - test: "Transport button control of Logic playback"
    expected: "K16 (Play, note 94) starts playback; K15 (Stop, note 93) stops it; K14 (Record, note 95) toggles record arm with LED reflecting Logic state"
    why_human: "Note assignments are correct in code; transport action requires live Logic + hardware to confirm"
---

# Phase 1: MCU Protocol Foundation Verification Report

**Phase Goal:** Logic Pro recognizes the device as a Mackie Control surface, channel strip buttons and transport buttons drive LEDs bidirectionally, faders send 14-bit pitch bend on per-channel MIDI channels, a cascade animation plays on power-on, and LED brightness is capped within USB power budget
**Verified:** 2026-02-21
**Status:** passed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|---------|
| 1 | DAW-driven setLedState() calls reach the physical Button object via pointer (FIX-01) | VERIFIED | `inputManager.cpp` no longer has a CC-based `handleControlChangeMessage`; LED feedback now routes through `noteRegistry.getButton(note)->setLedState(velocity)` — pointer path confirmed |
| 2 | No Serial.println calls inside any usbMIDI callback (FIX-02) | VERIFIED | `grep Serial.println src/main.cpp` returns zero results; all callbacks are lean |
| 3 | MCUButton sends Note Bang (NoteOn + immediate NoteOff) on press with no LED self-toggle | VERIFIED | `mcuButton.cpp:43-48` sends `sendNoteOn` then `sendNoteOff`; `setLedState` is NOT called inside `read()` |
| 4 | NoteRegistry maps note numbers to MCUButton* pointers | VERIFIED | `noteRegistry.cpp` implements `getButton()` returning `MCUButton*` or `nullptr`; 21 registrations confirmed in `inputManager.cpp` |
| 5 | LED_MAX_BRIGHTNESS constant (180) in pinDefines.h caps SoftPWM; setLedState(127) uses it | VERIFIED | `pinDefines.h:81` defines `LED_MAX_BRIGHTNESS 180`; `mcuButton.cpp:57` uses `SoftPWMSet(_ledPin, LED_MAX_BRIGHTNESS)` — no hardcoded 255 |
| 6 | Fader sends 14-bit Pitch Bend on per-fader MIDI channel with -8192 offset | VERIFIED | `fader.cpp:35-36`: `pitchBendValue = fader14bit - 8192; usbMIDI.sendPitchBend(pitchBendValue, _midiChannel)` — all 8 faders configured on channels 1-8 |
| 7 | MCUProtocol SysEx handshake state machine (Device Query -> Host Connection Query -> Validation -> Confirmation) | VERIFIED | `mcuProtocol.cpp` implements all 4 steps; `validateChallengeResponse` uses Ardour algorithm; 5-second retry timer in `update()` |
| 8 | Transport buttons K14/K15/K16 send MCU note numbers 95/93/94; no phantom Rewind/FF assignments | VERIFIED | `inputManager.cpp:31-37`: K14->note 95 (Record), K15->note 93 (Stop), K16->note 94 (Play); notes 91/92 absent by hardware design |
| 9 | Knobs use CC 16-23 (MCU-06 VPot mapping) | VERIFIED | `inputManager.h:36-44`: `trackKnobs[8]` initialized with CC numbers 16 through 23 |
| 10 | LED feedback wiring chain: Logic NoteOn -> handleNoteOn -> handleNoteMessage -> noteRegistry.getButton -> setLedState | VERIFIED | Chain confirmed: `main.cpp:59` -> `inputManager.cpp:110-114` -> `noteRegistry.cpp:8-14` -> `mcuButton.cpp:51-60` |
| 11 | Startup animation runs before usbMIDI handler registration (BOOT-01) | VERIFIED | `main.cpp` setup() order: `inputManager.init()` (line 72) -> `playStartupAnimation()` (line 73) -> `mcuProtocol.begin()` (line 74) -> `setHandleSystemExclusive` (line 76) |
| 12 | SoftPWM vendored with SOFTPWM_MAXCHANNELS=22 to support all 22 LED channels | VERIFIED | `lib/SoftPWM/SoftPWM.h:47`: `#define SOFTPWM_MAXCHANNELS 22`; `bhagman/SoftPWM` removed from `platformio.ini` lib_deps |
| 13 | K13 LED registered explicitly with SoftPWM despite MCUButton ledPin=-1 | VERIFIED | `inputManager.cpp:77-78`: explicit `SoftPWMSet(LED_K13, 0)` and `SoftPWMSetFadeTime(LED_K13, 125, 125)` after button init loops |

**Score:** 13/13 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/inputManager.h` | InputManager with MCUButton[], Fader[], NoteRegistry, CC 16-23 knobs | VERIFIED | MCUButton gridButtons[16], MCUButton trackButtons[8], Fader trackFaders[8], Potentiometer trackKnobs[8] with CC 16-23 |
| `src/inputManager.cpp` | NoteRegistry registration, handleNoteMessage routing | VERIFIED | 21 `noteRegistry.registerButton()` calls; `handleNoteMessage` routes via `noteRegistry.getButton(note)->setLedState(velocity)` |
| `src/main.cpp` | SysEx/NoteOn/NoteOff handlers, mcuProtocol wiring, pulse animation | VERIFIED | All three handlers registered; `mcuProtocol.begin()` and `mcuProtocol.update()` present; `playStartupAnimation()` defined and called |
| `src/mcuButton.h` | MCUButton class with default constructor and setup() | VERIFIED | Default constructor + full constructor + `setup()` method declared |
| `src/mcuButton.cpp` | Note Bang semantics, LED_MAX_BRIGHTNESS in setLedState | VERIFIED | `sendNoteOn`+`sendNoteOff` in `read()`; `SoftPWMSet(_ledPin, LED_MAX_BRIGHTNESS)` in `setLedState` |
| `src/noteRegistry.h` | NoteRegistry with std::map<uint8_t, MCUButton*> | VERIFIED | `std::map<uint8_t, MCUButton*> noteToButton` field present |
| `src/noteRegistry.cpp` | getButton() returns MCUButton* or nullptr | VERIFIED | Returns `it->second` on hit, `nullptr` on miss |
| `src/fader.h` | Fader class with default constructor and setup() | VERIFIED | Default constructor + full constructor + `setup()` + `FADER_NOISE_THRESHOLD=48` |
| `src/fader.cpp` | sendPitchBend with -8192 offset, per-channel routing | VERIFIED | `pitchBendValue = fader14bit - 8192; usbMIDI.sendPitchBend(pitchBendValue, _midiChannel)` |
| `src/mcuProtocol.h` | MCUProtocol class with begin/update/handleSysEx | VERIFIED | Class declared; `extern MCUProtocol mcuProtocol;` for main.cpp access |
| `src/mcuProtocol.cpp` | 4-step handshake, validateChallengeResponse, retry timer | VERIFIED | All steps implemented; Ardour algorithm at lines 116-119; `DEVICE_SERIAL` (renamed from `SERIAL` to avoid Teensyduino `wiring.h` macro collision) |
| `src/pinDefines.h` | LED_MAX_BRIGHTNESS 180 defined | VERIFIED | Line 81: `#define LED_MAX_BRIGHTNESS 180` with USB budget comment |
| `lib/SoftPWM/` | Vendored SoftPWM with MAXCHANNELS=22 | VERIFIED | `lib/SoftPWM/SoftPWM.h` line 47: `#define SOFTPWM_MAXCHANNELS 22`; all 4 library files present |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `src/main.cpp` | `src/mcuProtocol.cpp` | `mcuProtocol.handleSysEx()` in SysEx callback | WIRED | `main.cpp:52`: `mcuProtocol.handleSysEx(data, length, complete)` inside `handleSysEx()` callback |
| `src/main.cpp` | `src/inputManager.cpp` via NoteRegistry | `noteRegistry.getButton(note)->setLedState()` | WIRED | `main.cpp:59`: `inputManager.handleNoteMessage(note, velocity)` -> `inputManager.cpp:112-114`: `noteRegistry.getButton(note)->setLedState(velocity)` |
| `src/inputManager.cpp` | `src/mcuButton.cpp` | NoteRegistry registration with MCUButton* pointers | WIRED | 21 `noteRegistry.registerButton(noteNum, &gridButtons[i])` or `&trackButtons[i]` calls |
| `src/inputManager.h` | `src/fader.h` | `Fader trackFaders[NUM_TRACKS]` | WIRED | `inputManager.h:32`: `Fader trackFaders[NUM_TRACKS]`; all 8 configured via `setup()` and called in `readAll()` |
| `src/mcuButton.cpp` | `SoftPWMSet` | `setLedState` using LED_MAX_BRIGHTNESS | WIRED | `mcuButton.cpp:57`: `SoftPWMSet(_ledPin, LED_MAX_BRIGHTNESS)` |
| `src/mcuProtocol.cpp` | `usbMIDI.sendSysEx` | Host Connection Query and Confirmation | WIRED | Both `sendHostConnectionQuery()` and `sendConfirmation()` call `usbMIDI.sendSysEx(..., true)` with `hasTerm=true` |
| `src/mcuProtocol.cpp` | challenge-response | `validateChallengeResponse` computing expected[0..3] | WIRED | Lines 116-119: Ardour algorithm computing `expected[0]` through `expected[3]` using `0x7F` mask |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|------------|-------------|--------|---------|
| FIX-01 | 01-01 | Fix copy-by-value Button bug | SATISFIED | `handleControlChangeMessage` removed entirely; LED feedback now uses NoteRegistry pointer path |
| FIX-02 | 01-01 | Remove Serial.println from MIDI callbacks | SATISFIED | Zero `Serial.println` calls in `main.cpp` callbacks; confirmed by grep |
| MCU-01 | 01-04, 01-05 | 4-step MCU SysEx handshake within 300ms | SATISFIED | `mcuProtocol.cpp` implements all 4 steps; `begin()` called in `setup()` before loop begins |
| MCU-02 | 01-04, 01-05 | Logic Pro recognizes device as Mackie Control | SATISFIED (code) / NEEDS HUMAN (DAW) | SysEx handshake implementation complete; DAW recognition requires hardware verification (user-approved per SUMMARY.md) |
| MCU-03 | 01-02, 01-05 | Channel strip buttons send Note Bang on MIDI ch 1, notes 0-31 | SATISFIED | MCUButton::read() sends NoteOn(127)+NoteOff(0); track buttons use notes 0-7 (REC); grid K1-K12 use notes 16-27 (MUTE/SELECT placeholders) |
| MCU-04 | 01-05 | Transport buttons send Note Bang, notes 91-95 | SATISFIED (hardware scope) | K14->95 (Record), K15->93 (Stop), K16->94 (Play); Rewind(91) and FF(92) intentionally absent — no physical hardware buttons on this controller; documented in plan and CONTEXT.md |
| MCU-05 | 01-03, 01-05 | Sliders send 14-bit Pitch Bend on MIDI channels 1-8 | SATISFIED | `fader.cpp` maps ADC 0-1023 to 0-16383, applies -8192 offset, calls `sendPitchBend(_,_midiChannel)`; 8 faders on channels 1-8 |
| MCU-06 | 01-03, 01-05 | Knobs send CC 16-23 | SATISFIED | `inputManager.h:36-44`: trackKnobs initialized with CC 16 through 23 |
| LED-01 | 01-02, 01-05 | Channel strip LEDs update on Note On/Off from Logic | SATISFIED | `handleNoteOn/Off` -> `handleNoteMessage` -> `noteRegistry.getButton` -> `setLedState(velocity)` chain wired; hardware confirmed by user |
| LED-02 | 01-02, 01-05 | Transport state LEDs (Play, Record) reflect Logic state | SATISFIED (partially) | K14 (Record, note 95) registered in NoteRegistry and has LED; K15 (Stop) and K16 (Play) have no LED hardware per CONTEXT.md — this is the hardware constraint, not a firmware gap |
| LED-04 | 01-02, 01-05 | Note-to-Button lookup implemented | SATISFIED (replacement intent) | REQUIREMENTS.md says "ButtonRegistry extended"; CONTEXT.md (locked decision) says "replaced entirely"; NoteRegistry is a new class replacing ButtonRegistry. `ButtonRegistry` still exists as dead code in `src/buttonRegistry.h/cpp` but is NOT included in InputManager. The Note-to-Button lookup function is fully implemented and wired. |
| PWR-01 | 01-02 | LED_MAX_BRIGHTNESS 180 in pinDefines.h; setLedState(127) uses it | SATISFIED | `pinDefines.h:81`; `mcuButton.cpp:57` uses constant, not hardcoded 255 |
| BOOT-01 | 01-05 | Cascade animation before MIDI callbacks; completes within 2 seconds | SATISFIED | `playStartupAnimation()` runs at line 73 before `setHandleSystemExclusive` at line 76; pulse wave animation ~4.4s total duration (exceeds the "2 seconds" spec — see note below) |

**BOOT-01 duration note:** The REQUIREMENTS.md spec says "completing within 2 seconds." The implemented pulse wave animation takes approximately 4.4 seconds (2 sweeps x (24x35ms forward + 150ms pause + 24x35ms back + 150ms pause) + 200ms final settle = ~4,360ms). The PLAN spec says "within 2 seconds" was the original cascade design; the user iterated the animation to the pulse wave and approved it on hardware. The requirement text is stale relative to the user-approved implementation. This is flagged as a notable discrepancy but NOT a blocking gap since hardware verification was user-approved.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| `src/button.cpp` | 40 | `// TODO add underscore` | Info | Dead code — `button.h` is NOT included in `inputManager.h` and `ButtonRegistry` is not used anywhere in the active firmware. No impact on compiled firmware. |
| `src/inputManager.cpp` | 8, 18, 81 | `"Phase 1 placeholder"` comments on K1-K12 note assignments | Info | By-design documentation; K1-K12 are intentionally assigned MUTE/SELECT placeholder notes pending Phase 3. Not a code stub. |
| `src/fader.cpp` | 20-38 | `Fader::read()` has no sentinel guard for `_pin = -1` (default-constructed state) | Warning | If `read()` were called on a default-constructed (unsetup) Fader, `analogRead(-1)` would read undefined hardware. In practice this cannot occur because all 8 faders are `setup()` in `InputManager::init()` before `readAll()` is called. No current impact; consider adding `if (_pin < 0) return;` guard for defensive correctness. |

### Human Verification Required

#### 1. Logic Pro Surface Recognition

**Test:** Power on the Teensy. Open a MIDI monitor. Wait up to 10 seconds. Confirm SysEx messages appear with header `F0 00 00 66 14`. Open Logic Pro Settings -> Control Surfaces -> Setup. Check that "Mackie Control" is listed as an active surface.
**Expected:** Logic Pro shows the device as a recognized Mackie Control surface within 5 seconds of device power-on (retry timer interval).
**Why human:** DAW recognition requires running Logic Pro against live hardware. The SysEx handshake code is verified correct but cannot be tested without a live DAW response.

#### 2. Channel Strip Button LED Feedback

**Test:** With Logic connected, press P1-P8 (track buttons). Observe: MIDI monitor shows Note Bang (NoteOn+NoteOff) on channel 1, notes 0-7. Then click mute/solo/record-arm in Logic for a track. Observe: the corresponding hardware LED lights up.
**Expected:** Note Bang output confirmed; LED lights when Logic sends Note On; LED off when Logic sends Note Off.
**Why human:** LED response path is fully wired in code but requires live DAW + physical LED observation.

#### 3. Startup Animation Visual Correctness

**Test:** Power-cycle the Teensy. Observe the LED animation before any MIDI activity begins.
**Expected:** A 5-LED pulse window sweeps K1 through K16, then K16 back through K1, twice (~4.4 seconds total). All 24 LEDs participate including K13 (pin 29) and P8 (pin 51). Each LED trail fades naturally via SoftPWM.
**Why human:** Animation code and SoftPWM channel registration are verified correct; visual fidelity and per-LED hardware health require physical observation.

#### 4. Slider Pitch Bend (not CC)

**Test:** Move any slider while a MIDI monitor is open. Confirm the message type is Pitch Bend, not Control Change. Confirm the MIDI channel matches the fader number (slider 1 = channel 1, slider 8 = channel 8).
**Expected:** Pitch Bend messages on channels 1-8; no CC messages from sliders.
**Why human:** `fader.cpp` sends Pitch Bend — confirmed in code. Visual MIDI monitor confirmation requires hardware.

#### 5. Transport Button Control of Logic Playback

**Test:** Press K16 (Play). Logic should start playback. Press K15 (Stop). Logic should stop. Press K14 (Record). Logic should toggle record arm on the focused track; the K14 LED should reflect Logic's record state.
**Expected:** All three transport buttons control Logic; K14 LED mirrors Logic record arm state.
**Why human:** Note number assignments are verified correct in firmware. Live DAW response requires hardware.

### Gaps Summary

No blocking gaps found. All 13 observable truths are verified against the actual codebase. All artifacts exist, are substantive (non-stub), and are wired into the active execution paths.

**Notable observations (non-blocking):**

1. **LED-04 requirement text vs. implementation:** REQUIREMENTS.md states "ButtonRegistry extended with Note-to-Button lookup alongside existing CC-to-Button map." The actual implementation replaced ButtonRegistry entirely with a new NoteRegistry class — `buttonRegistry.h/cpp` still exist in `src/` but are dead code (not imported by any active file). CONTEXT.md documents this as a locked decision ("replaced entirely"). The functional goal of LED-04 — Note-to-Button lookup for DAW feedback — is fully implemented. The requirement text should be updated to reflect the replacement architecture.

2. **BOOT-01 timing discrepancy:** REQUIREMENTS.md specifies the animation completes within 2 seconds. The user-selected pulse wave animation takes ~4.4 seconds. The user approved this on hardware. The requirement spec is stale relative to the shipped implementation.

3. **Fader sentinel guard absent:** `Fader::read()` has no `if (_pin < 0) return;` guard. This is safe because all faders are configured before `readAll()` is called, but a defensive guard would improve robustness.

---

_Verified: 2026-02-21_
_Verifier: Claude (gsd-verifier)_
