# Architecture Research

**Domain:** Teensy USB MIDI controller firmware — Mackie Control Universal protocol layer
**Researched:** 2026-02-21
**Confidence:** HIGH (MCU protocol constants verified via Control Surface library official docs; pickup mode pattern verified via Arduino forum and community implementations; elapsedMillis pattern verified via PJRC official docs)

## Standard Architecture

### System Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                      main.cpp (Entry Point)                      │
│  setup() → init all layers, register usbMIDI callbacks          │
│  loop()  → readAll(), usbMIDI.read(), animationManager.tick()   │
└────────────────────────┬────────────────────────────────────────┘
                         │
         ┌───────────────┼───────────────┐
         │               │               │
┌────────▼────────┐ ┌───▼────────┐ ┌───▼──────────────┐
│  InputManager   │ │ MCULayer   │ │ AnimationManager  │
│  (existing)     │ │  (NEW)     │ │  (NEW)            │
│                 │ │            │ │                   │
│ - Button[]      │ │ - Handshake│ │ - StartupAnim     │
│ - Potentiometer │ │ - SysEx    │ │ - BeatChaser      │
│   (+ pickup)    │ │ - VPot CC  │ │ - ActionAnims     │
│ - ButtonRegistry│ │ - Transport│ │ - ShiftGlow       │
└────────┬────────┘ └───┬────────┘ └───┬──────────────┘
         │              │              │
┌────────▼──────────────▼──────────────▼──────────────┐
│              Hardware Abstraction Layer               │
│   Button (Bounce2 + SoftPWM)    Potentiometer (ADC)  │
└─────────────────────────────────────────────────────┘
         │                    │
┌────────▼────────┐  ┌────────▼────────┐
│  usbMIDI        │  │  SoftPWM        │
│  (Teensy SDK)   │  │  (LED control)  │
└─────────────────┘  └─────────────────┘
```

### Component Responsibilities

| Component | Responsibility | Communicates With |
|-----------|----------------|-------------------|
| `main.cpp` | Entry point, callback registration, loop orchestration | InputManager, MCULayer, AnimationManager, usbMIDI |
| `InputManager` | Poll all hardware inputs, route DAW feedback | Button[], Potentiometer[], ButtonRegistry, MCULayer |
| `MCULayer` | MCU protocol: SysEx handshake, Note messages for transport/buttons, VPot CC feedback, fader pitch bend | usbMIDI, InputManager, ButtonRegistry |
| `AnimationManager` | Startup sequences, beat chaser, action animations, shift glow | Button[], SoftPWM directly, clock counter |
| `Button` | Switch debounce + LED PWM pair, LED brightness/blink state | Bounce2, SoftPWM, usbMIDI |
| `Potentiometer` | ADC read + noise filter + pickup mode state | ADC pin, usbMIDI |
| `ButtonRegistry` | CC/Note → Button* lookup for DAW LED feedback | Button* |
| `ClockCounter` | 24 PPQN counter → beat tick signal, transport state | AnimationManager |

## Recommended Project Structure

```
src/
├── main.cpp                # Entry point (minimal changes — add AnimationManager.tick())
├── pinDefines.h            # Existing — no changes needed
├── button.h/cpp            # Extend: add setLedBlink(), setLedBrightness(0-255)
├── potentiometer.h/cpp     # Extend: add pickup mode state per instance
├── buttonRegistry.h/cpp    # Extend: support Note number lookup in addition to CC
├── inputManager.h/cpp      # Extend: sendMCUMessages() replacing CC sends
├── mcuLayer.h/cpp          # NEW: SysEx handshake, protocol message dispatch
├── animationManager.h/cpp  # NEW: startup animations + beat chaser + action anims
└── clockCounter.h/cpp      # NEW: 24 PPQN counter, transport run/stop state
```

### Structure Rationale

- **`mcuLayer`** is a distinct module because the MCU protocol is a separate concern from hardware input reading. It handles protocol-level handshake, VPot ring LED CC format, transport Note mapping, and SysEx parsing. Keeping it isolated means `InputManager` does not need to know about MCU specifics.
- **`animationManager`** is separated from `InputManager` because animations are time-driven (elapsedMillis), not event-driven per input poll. It needs to call `SoftPWMSet()` on LED pins directly and independently of whether buttons are being pressed.
- **`clockCounter`** is a tiny but distinct module because its MIDI clock callback increments a counter that multiple consumers (AnimationManager beat chaser, transport state LEDs) need to query.

## Architectural Patterns

### Pattern 1: MCU SysEx Handshake — Host-Initiated, 300ms Window

**What:** When Logic Pro boots, it sends a Device Query SysEx to all MIDI ports. The firmware must respond with a Host Connection Query (containing a device serial + 4-byte challenge), then compute a challenge-response and reply within 300ms. Logic confirms with Host Connection Confirmation (or Error).

**When to use:** Required once at startup for Logic Pro to recognize the device as an MCU surface. Without this, Logic ignores all subsequent transport/button messages.

**Protocol bytes (confirmed HIGH confidence via NicoG60/TouchMCU docs):**
```
Header: F0 00 00 66 14  (Mackie manufacturer ID + MCU product ID)

1. Logic → Device:  F0 00 00 66 14 00 F7          (Device Query)
2. Device → Logic:  F0 00 00 66 14 01 [serial 7B] [challenge 4B] F7
3. Logic → Device:  F0 00 00 66 14 02 [serial 7B] [response 4B] F7
4. Device → Logic:  F0 00 00 66 14 03 F7           (Confirmation)
   or               F0 00 00 66 14 04 F7           (Error)

Challenge-response algorithm (XOR + bitwise, verified via libMackieControl docs):
  r[0] = 0x7F & (c[0] + (c[1] ^ 0x0A) - c[3]);
  r[1] = 0x7F & ((c[2] >> 4) ^ (c[0] + c[3]));
  r[2] = 0x7F & (c[3] - (c[2] << 2) ^ (c[0] | c[1]));
  r[3] = 0x7F & (c[1] - c[2] + (0xF0 ^ (c[3] << 4)));
```

**Implementation in MCULayer:**
```cpp
// mcuLayer.h
class MCULayer {
public:
    void handleSysEx(const byte* data, unsigned int length);
    void sendHostConnectionQuery();
    void sendHostConnectionConfirmation();
private:
    bool _connected = false;
    byte _challengeBytes[4];
    void computeResponse(const byte* challenge, byte* response);
};
```

**Trade-offs:** The 300ms timing window is strict. The Teensy runs at 72 MHz — computing the response inline in the SysEx callback is safe. Do NOT defer to the loop().

### Pattern 2: MCU Protocol MIDI Message Map

**What:** The MCU protocol uses specific MIDI message types for each control category. Replace the existing CC-for-everything approach with the correct MCU message types. (Confidence: HIGH — verified via tttapa Control-Surface library constants and libMackieControl docs)

```
Control Category       MIDI Type        Numbers
─────────────────────  ───────────────  ────────────────────────────
Channel faders 1-8     Pitch Bend       MIDI channels 1-8
Master fader           Pitch Bend       MIDI channel 9
Channel rec arm 1-8    Note On/Off      notes 0-7   (vel 0=off, 127=on)
Channel solo 1-8       Note On/Off      notes 8-15
Channel mute 1-8       Note On/Off      notes 16-23
Channel select 1-8     Note On/Off      notes 24-31
VPot click 1-8         Note On/Off      notes 32-39
Transport: Rewind      Note On/Off      note 91
Transport: FastFwd     Note On/Off      note 92
Transport: Stop        Note On/Off      note 93
Transport: Play        Note On/Off      note 94
Transport: Record      Note On/Off      note 95
Cursor Up/Down/L/R     Note On/Off      notes 96-99
Zoom                   Note On/Off      note 100
VPot ring LED 1-8      CC 48-55        value = (mode<<4 | position)
Fader touch 1-8        Note On/Off      notes 104-111

LED "Note Bang" semantics (host → device):
  velocity 0x00 = LED off
  velocity 0x01 = LED blink
  velocity 0x7F = LED solid on
```

**Impact on ButtonRegistry:** The existing registry maps CC numbers to Button pointers. Extend it to also map MIDI note numbers to Button pointers. Add `noteNumToButton: std::map<int, Button*>`. Route incoming Note messages from MCULayer to ButtonRegistry for LED state updates.

### Pattern 3: Pickup Mode — Per-Potentiometer State Machine

**What:** Each `Potentiometer` instance tracks whether its physical position is "in sync" with the DAW's last known value. When a bank switch or external DAW change causes the DAW value to diverge from the physical position, the potentiometer enters `WAITING` state and suppresses CC output until the physical value crosses through the DAW value.

**When to use:** Any time a fader or knob is controlling a parameter that can change externally (bank switch, automation playback, remote control).

**State machine (2 states per pot):**
```
          DAW sends new value != physical pos
ACTIVE ──────────────────────────────────────► WAITING
  ▲                                               │
  │                                               │
  │   physical crosses through DAW value          │
  └───────────────────────────────────────────────┘

ACTIVE:  Read ADC → send MIDI CC/PitchBend
WAITING: Read ADC → suppress MIDI → update LED blink on paired button
         When physical value crosses DAW value: transition to ACTIVE, clear blink
```

**Implementation in Potentiometer (extend existing class):**
```cpp
// potentiometer.h additions
enum class PickupState : uint8_t {
    ACTIVE,   // in sync — send MIDI normally
    WAITING   // out of sync — suppress MIDI until crossover
};

class Potentiometer {
public:
    // existing constructor/init/read unchanged externally
    void setDawValue(int dawValue);     // called when DAW sends fader update
    bool isWaiting() const;            // queried by InputManager to drive LED blink
private:
    PickupState _pickupState = PickupState::ACTIVE;
    int _dawValue = 0;                 // last known DAW value (0-127 or 0-16383 for pitch bend)
    int _lastPhysical = 0;             // last physical ADC reading
    bool crossedThrough(int oldPhys, int newPhys, int target);
};
```

**Crossover detection logic:**
```cpp
bool Potentiometer::crossedThrough(int oldPhys, int newPhys, int target) {
    // Returns true if the physical position moved through the target value
    return (oldPhys <= target && newPhys >= target) ||
           (oldPhys >= target && newPhys <= target);
}

void Potentiometer::read() {
    int raw = analogRead(_pin);
    if (!hasChanged(raw)) return;

    if (_pickupState == PickupState::WAITING) {
        if (crossedThrough(_lastPhysical, raw, _dawValue)) {
            _pickupState = PickupState::ACTIVE;
            // InputManager will clear blink on paired button
        }
        _lastPhysical = raw;
        return; // suppress MIDI output
    }

    // ACTIVE: send MIDI normally
    _lastPhysical = raw;
    int midiValue = map(raw, 0, 1023, _invert ? 127 : 0, _invert ? 0 : 127);
    usbMIDI.sendControlChange(_ccNum, midiValue, 1);
}
```

**Trade-offs:** The `crossedThrough` check requires storing `_lastPhysical` separately from the existing `lastReading`. This is a minor addition. The paired button blink is owned by `InputManager` — it polls `isWaiting()` on each potentiometer each loop and calls `button.setLedBlink(true/false)` on the corresponding channel button.

### Pattern 4: MIDI Clock Counter — 24 PPQN → Beat Tick

**What:** Logic Pro sends 24 MIDI clock pulses per quarter note during playback. A `ClockCounter` module counts these pulses and exposes a "beat elapsed" flag that `AnimationManager` consumes to advance the beat chaser.

**Implementation (new `clockCounter.h/cpp`):**
```cpp
class ClockCounter {
public:
    void tick();            // called from handleClock() in main.cpp
    void start();           // called from handleStart()
    void stop();            // called from handleStop()
    bool beatElapsed();     // returns true once per beat, then clears
    bool isRunning() const { return _running; }
    int  pulseCount() const { return _pulseCount; }

private:
    static const int PULSES_PER_BEAT = 24;
    volatile int _pulseCount = 0;
    volatile bool _running = false;
    volatile bool _beatFlag = false;
};

void ClockCounter::tick() {
    if (!_running) return;
    _pulseCount++;
    if (_pulseCount >= PULSES_PER_BEAT) {
        _pulseCount = 0;
        _beatFlag = true;
    }
}

bool ClockCounter::beatElapsed() {
    if (_beatFlag) { _beatFlag = false; return true; }
    return false;
}
```

**Integration in main.cpp:**
```cpp
ClockCounter clockCounter;

void handleClock() { clockCounter.tick(); }
void handleStart() { clockCounter.start(); }
void handleStop()  { clockCounter.stop();  }
```

**AnimationManager polls:**
```cpp
void AnimationManager::tick() {
    if (clockCounter.beatElapsed()) advanceBeatChaser();
}
```

**Note:** `usbMIDI` callbacks are called from `usbMIDI.read()` in the main loop — they are NOT interrupt-driven on Teensy USB MIDI. `volatile` is included for safety but the main loop is single-threaded. Do NOT put heavy work in clock callbacks; just increment the counter.

### Pattern 5: Animation State Machine — Non-Blocking, elapsedMillis Driven

**What:** A single `AnimationManager` class owns all animation state. It uses `elapsedMillis` timers (Teensy built-in) for non-blocking sequencing. Each animation type is a state in an outer enum, with substates for step-by-step progression.

**Outer state enum:**
```cpp
enum class AnimationState : uint8_t {
    STARTUP,          // plays once at power-on, then → IDLE
    IDLE,             // no animation; reflect DAW state
    BEAT_CHASER,      // active during playback; advances on beat
    RECORD_PULSE,     // record button pulses while recording
    BANK_SWEEP,       // brief cascade on bank switch
    SHIFT_GLOW        // dimmed glow on non-active LEDs while shift held
};
```

**Startup animation substates (separate enum per style):**
```cpp
enum class StartupStyle : uint8_t { CASCADE, RIPPLE, SPARKLE, FLASH };
// Compile-time config:
#define STARTUP_ANIMATION_STYLE StartupStyle::CASCADE
```

**AnimationManager structure:**
```cpp
class AnimationManager {
public:
    void begin();           // called from setup() — triggers STARTUP
    void tick();            // called every loop() iteration
    void onPlay();          // transition to BEAT_CHASER
    void onStop();          // transition to IDLE
    void onRecord(bool active);
    void onBankSwitch();
    void onShift(bool held);

private:
    AnimationState _state = AnimationState::STARTUP;
    StartupStyle   _startupStyle = STARTUP_ANIMATION_STYLE;
    int            _startupStep = 0;
    int            _beatChaserPos = 0;  // 0-15 (grid position)
    bool           _recordActive = false;
    bool           _shiftHeld = false;

    elapsedMillis  _stepTimer;          // generic step sequencer timer
    elapsedMillis  _blinkTimer;         // for pulse/blink effects

    // References to Button arrays (set during begin())
    Button** _gridButtons;   // pointer to InputManager's grid array
    Button** _trackButtons;

    void tickStartup();
    void tickBeatChaser();
    void tickRecordPulse();
    void tickBankSweep();
    void tickShiftGlow();
};
```

**Non-blocking step pattern (cascade example):**
```cpp
void AnimationManager::tickStartup() {
    const uint32_t STEP_INTERVAL_MS = 40;
    if (_stepTimer >= STEP_INTERVAL_MS) {
        _stepTimer -= STEP_INTERVAL_MS;  // subtract to preserve schedule
        if (_startupStep < 16) {
            SoftPWMSet(_gridButtons[_startupStep]->getLEDPin(), 255);
            _startupStep++;
        } else {
            _state = AnimationState::IDLE;
            _startupStep = 0;
            // Settle: request DAW state refresh (MCULayer sends query)
        }
    }
}
```

**Trade-offs:** Using `elapsedMillis` (Teensy built-in, no interrupt overhead) is the correct pattern here. Do NOT use `delay()`. Multiple `elapsedMillis` instances can run simultaneously for overlapping effects (e.g., shift glow + blink LED). The Teensy docs confirm `>= INTERVAL` comparison (never `==`) to handle loop latency gracefully.

## Data Flow

### Outbound: Hardware Input → DAW (MCU Protocol)

```
User presses grid button (row 4, col 3 = "Record")
    ↓
Button::read() detects Bounce2 fell() edge
    ↓
InputManager maps button index → MCU note number (note 95)
    ↓
usbMIDI.sendNoteOn(95, 127, 1)   [Note Bang: immediately follow with NoteOff]
usbMIDI.sendNoteOff(95, 0, 1)
    ↓
Logic Pro receives Note 95 → toggles record arm

User moves slider 1
    ↓
Potentiometer::read() → pickup state check
    WAITING: suppress output, check crossover
    ACTIVE: map ADC 0-1023 → pitch bend 0-16383
    ↓
usbMIDI.sendPitchBend(mappedValue, 1)  [channel 1 = fader 1]
    ↓
Logic Pro receives pitch bend on ch1 → moves channel 1 fader
```

### Inbound: DAW → Hardware (LED Feedback)

```
Logic Pro sends Note On (note 0, vel 127) → channel 1 rec arm LED on
    ↓
usbMIDI.read() → handleNoteOnMessage callback in main.cpp
    ↓
MCULayer::handleNoteOn(note=0, vel=127)
    ↓
ButtonRegistry::lookupNote(0) → Button* recButton1
    ↓
recButton1->setLedState(true)   [vel 127 = solid on]
recButton1->setLedBlink(false)  [vel 1 = blink; vel 0 = off]
    ↓
SoftPWMSet(ledPin, 255)

Logic Pro sends CC 48, value 0x25 → VPot ring 1 update
    ↓
MCULayer::handleControlChange(cc=48, val=0x25)
    ↓
Decoded: mode = (0x25 >> 4) & 0x03 = 2 (fill center out)
         position = 0x25 & 0x0F = 5
    ↓
[VPot ring LEDs not present on this hardware — store value, ignore LEDs]

Logic Pro sends Pitch Bend (ch1, val=8192) → fader 1 at center
    ↓
MCULayer::handlePitchBend(channel=1, value=8192)
    ↓
potentiometer[0].setDawValue(map(8192, 0, 16383, 0, 1023))
→ if physical != daw value: potentiometer[0] enters WAITING state
→ InputManager detects isWaiting() → trackButton[0].setLedBlink(true)
```

### MIDI Clock → Beat Animation

```
Logic Pro sends 24x MIDI Clock per beat during playback
    ↓
handleClock() → clockCounter.tick()
    ↓
clockCounter: _pulseCount++ → at 24: _beatFlag = true, _pulseCount = 0
    ↓
AnimationManager::tick() → if (clockCounter.beatElapsed()):
    _beatChaserPos = (_beatChaserPos + 1) % 16
    SoftPWMSet(prevGrid->getLEDPin(), prevState)  // restore previous
    SoftPWMSet(currGrid->getLEDPin(), 255)        // light current
```

## Component Boundaries

### What MCULayer Owns (and What It Does Not Own)

MCULayer owns:
- SysEx parsing and handshake state machine
- Mapping MCU note numbers to semantic actions (play pressed, rec arm ch1 on, etc.)
- Mapping incoming pitch bend to fader index
- Decoding VPot ring CC values
- All usbMIDI send calls for outbound MCU protocol messages

MCULayer does NOT own:
- Physical button debouncing (Button/Bounce2)
- LED PWM output (SoftPWM — owned by Button class)
- Animation timing (AnimationManager)
- Which physical pin maps to which hardware (pinDefines.h)

### What AnimationManager Owns (and What It Does Not Own)

AnimationManager owns:
- All elapsedMillis timers
- Animation state enum transitions
- Beat chaser position counter
- Startup step counter and style selection
- Direct SoftPWMSet() calls on LED pins during animation sequences

AnimationManager does NOT own:
- Whether a button is pressed (polling stays in InputManager)
- Transport start/stop events (it receives callbacks from main.cpp via onPlay()/onStop())
- Clock counting (delegates to ClockCounter)
- LED state during IDLE (those are owned by ButtonRegistry/DAW feedback)

### What InputManager Owns After Extension

InputManager gains:
- Polling `potentiometer[i].isWaiting()` each loop to drive blink on paired channel button
- Routing incoming MCU transport Note messages to AnimationManager callbacks
- Calling `mcuLayer.sendNoteOn/Off()` instead of `usbMIDI.sendControlChange()` for grid/track buttons

InputManager does NOT gain:
- Protocol-level MCU knowledge (that stays in MCULayer)
- Animation sequencing (that stays in AnimationManager)

## Build Order (What Must Be Implemented Before What)

```
Phase 1: MCU Protocol Foundation
  1a. mcuLayer.h/cpp — SysEx handshake (Logic must recognize device before anything else works)
  1b. Extend ButtonRegistry — add noteNumToButton map
  1c. Extend Button — add setLedBlink() using elapsedMillis + SoftPWM
  1d. main.cpp — register handleNoteOn/handlePitchBend/handleSysEx callbacks

  Verifiable: Logic Pro shows "Mackie Control" in control surface setup

Phase 2: Pickup Mode
  2a. Extend Potentiometer — add PickupState enum + _dawValue + crossedThrough()
  2b. Extend InputManager — poll isWaiting(), drive blink on paired channel button
  2c. MCULayer — route incoming pitch bend to potentiometer.setDawValue()

  Verifiable: Move slider, bank switch → blink appears; sweep fader through → blink clears

Phase 3: Grid Layout & Transport Mapping
  3a. Update InputManager — remap grid buttons 13-16 to MCU transport notes (93-95, shift)
  3b. Update InputManager — remap grid buttons 1-12 to MCU function notes
  3c. Implement shift layer state in InputManager (single bool _shiftHeld)

  Verifiable: Play/Stop/Record buttons control Logic transport

Phase 4: Animation Manager
  4a. clockCounter.h/cpp — 24 PPQN counter
  4b. animationManager.h/cpp — startup animations (pick one first: CASCADE)
  4c. Beat chaser — wire to clockCounter
  4d. Action animations — record pulse, bank sweep, shift glow

  Verifiable: Startup cascade visible; beat chaser moves with Logic playback BPM
```

## Anti-Patterns

### Anti-Pattern 1: Putting Heavy Work in MIDI Clock Callback

**What people do:** Advance animation frame, update all LEDs, compute beat position inside `handleClock()`.
**Why it's wrong:** On Teensy USB MIDI, `handleClock` is called from `usbMIDI.read()` in the main loop — it is synchronous, not ISR-driven. But at 120 BPM, clock arrives at 48 Hz. Doing SoftPWM updates inside the callback will corrupt the polling timing for buttons and potentiometers.
**Do this instead:** Set a flag (`_beatFlag = true`) in the callback. Do all LED work in `AnimationManager::tick()` which runs unconditionally in `loop()`.

### Anti-Pattern 2: Modifying ButtonRegistry to Use LED State Directly for Animations

**What people do:** Store animation brightness in the same `ledState` bool that ButtonRegistry uses for DAW feedback. Animation sets ledState = HIGH, DAW feedback overwrites it.
**Why it's wrong:** Animation and DAW state will fight. A DAW message arriving mid-animation will snap the LED to DAW state, causing a visual glitch. On the next animation tick, the LED snaps back.
**Do this instead:** AnimationManager writes directly to `SoftPWMSet()` using the LED pin (obtainable via `button.getLEDPin()`), bypassing `setLedState()`. During IDLE, DAW feedback owns the LED. During animations, AnimationManager owns it. Clear handoff on state transition.

### Anti-Pattern 3: Global Pickup Mode Flag Instead of Per-Potentiometer State

**What people do:** Use a single `bool pickupModeEnabled` that disables all pots at once during bank switch.
**Why it's wrong:** Different faders will cross the DAW value at different times as the user moves them. A global flag either requires all faders to be "picked up" before any send, or clears too early (the first fader to cross enables all).
**Do this instead:** Each `Potentiometer` instance holds its own `PickupState`. Faders re-enter ACTIVE independently when each one crosses its individual DAW target value.

### Anti-Pattern 4: Using `delay()` in Animation Sequences

**What people do:** `for (int i=0; i<16; i++) { setLED(i); delay(40); }` inside startup.
**Why it's wrong:** `delay()` blocks everything — button reads stop, USB MIDI stops, the MCU SysEx handshake 300ms window may be missed.
**Do this instead:** Use `elapsedMillis` + state machine. Each `loop()` iteration checks if the timer has elapsed and advances one step. Button reads and `usbMIDI.read()` continue uninterrupted.

### Anti-Pattern 5: Replacing ButtonRegistry with MCU Note Map and Losing CC Fallback

**What people do:** Delete the CC-to-Button map and only keep a Note-to-Button map after switching to MCU.
**Why it's wrong:** The existing CC mapping (102–117 grid, 20–27 track) is still used by non-MCU DAWs, and some testing workflows use CC messages directly.
**Do this instead:** Keep both maps in ButtonRegistry. The incoming message dispatcher (MCULayer for Notes, InputManager for CC) routes to the appropriate lookup. Zero-cost coexistence.

## Integration Points

### External (DAW ↔ Firmware)

| Direction | Message Type | Notes |
|-----------|-------------|-------|
| Firmware → DAW | Note Bang (Note On + immediate Note Off) | Transport, grid, track buttons |
| Firmware → DAW | Pitch Bend (ch 1-9) | Faders 1-8 + master |
| DAW → Firmware | Note Bang (vel 0/1/127) | LED on/blink/off for all buttons |
| DAW → Firmware | CC 48-55 | VPot ring LED values (hardware has none — parse and ignore or store) |
| DAW → Firmware | Pitch Bend | Fader position feedback (used to update pickup mode dawValue) |
| DAW → Firmware | SysEx (F0 00 00 66 14 ...) | Handshake + LCD updates (LCD not present — parse header, ignore content) |
| DAW → Firmware | MIDI Clock (0xF8) | 24 PPQN during playback for beat chaser |
| DAW → Firmware | Start (0xFA) / Stop (0xFC) | Transport state changes |

### Internal Boundaries

| Boundary | Communication | Notes |
|----------|---------------|-------|
| main.cpp ↔ MCULayer | Direct method calls in callbacks | `handleSysEx()`, `handleNoteOn()`, `handlePitchBend()` |
| main.cpp ↔ ClockCounter | Direct method calls | `tick()`, `start()`, `stop()` |
| MCULayer ↔ ButtonRegistry | Pointer lookup | `lookupNote(noteNum)` → `Button*` |
| MCULayer ↔ InputManager | Method call | `inputManager.setPotDawValue(index, value)` |
| InputManager ↔ AnimationManager | Callback or direct call | `animationManager.onPlay()`, `onStop()`, `onBankSwitch()` |
| AnimationManager ↔ Button | Direct pin access | `button.getLEDPin()` → `SoftPWMSet(pin, brightness)` |
| AnimationManager ↔ ClockCounter | Poll in tick() | `clockCounter.beatElapsed()` |

## Sources

- [NicoG60/TouchMCU — Mackie Control Protocol Documentation](https://github.com/NicoG60/TouchMCU/blob/main/doc/mackie_control_protocol.md) — HIGH confidence — primary protocol reference
- [tttapa/Control-Surface — MCU Note Constants](https://tttapa.github.io/Control-Surface/Doxygen/de/d44/group__MCU__Notes.html) — HIGH confidence — verified note number table
- [Do-sth-sharp/libMackieControl — MackieControl.md](https://github.com/Do-sth-sharp/libMackieControl/blob/main/doc/MackieControl.md) — HIGH confidence — challenge-response algorithm and VPot CC value format
- [PJRC — elapsedMillis Timing](https://www.pjrc.com/teensy/td_timing_elaspedMillis.html) — HIGH confidence — official Teensy timing docs
- [Arduino Forum — Fader Pickup/Catch Mode](https://forum.arduino.cc/t/fader-pickup-catch-mode/1206227) — MEDIUM confidence — implementation pattern discussion
- [Cycling '74 Forum — LogicControl Initialization](https://cycling74.com/forums/logiccontrol-and-maxmsp-initialization) — MEDIUM confidence — 300ms handshake window confirmed
- [Teensy Forum — Teensy 4.0 MCU Protocol](https://forum.pjrc.com/index.php?threads/teensy-4-0-midi-mackie-mcu-protocol.72181/) — MEDIUM confidence — practical Teensy MCU implementation patterns

---
*Architecture research for: Phaedr MIDI Controller — MCU Protocol + Animation Layer*
*Researched: 2026-02-21*
