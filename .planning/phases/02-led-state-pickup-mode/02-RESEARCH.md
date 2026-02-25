# Phase 2: LED State + Pickup Mode — Research

**Researched:** 2026-02-25
**Domain:** Embedded C++ FSM design on Teensy 3.5 — pickup mode state machines, millis()-based LED blinking, MIDI pitch bend receive for fader tracking, SoftPWM blink patterns
**Confidence:** HIGH (all implementation points grounded in the existing codebase; no new external libraries required; patterns carry forward directly from Phase 1)

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**Pickup trigger (reveal condition)**
- Out-of-sync state is NOT shown immediately on bank switch — no visual noise by default
- Out-of-sync indicators are revealed only when the user takes action on that channel strip: any button press, fader move, or pan pot move on that channel's physical controls
- Until triggered, the channel looks and behaves normally from the user's perspective

**Out-of-sync visual feedback**
- Channel strip button (P1–P8 for that channel) pulses to indicate out-of-sync state
- Grid row 1: one LED per channel indicates pan pot sync state
- Grid rows 2–3: one LED per slider indicates slider sync state (8 LEDs total, physically aligned to horizontal position of each slider)
- Pulse rate: medium (~2–3 Hz), symmetric (50/50 duty cycle)
- Pulse speed encodes distance from target: faster = farther from DAW value, slows as physical approaches target — gives "warmer/colder" feel without directional encoding
- All indicators for a channel go dark the moment that control picks up; no confirmation animation, no "all clear" flash

**Pickup crossover detection**
- Deadband: ±2–3 MIDI units around the DAW target value — compensates for ADC noise without feeling loose
- Either approach direction counts — physical does not need to cross from the "correct" side; entering the deadband from any direction triggers pickup
- Edge case: if physical position and DAW value are both at 0 or 127, pickup fires immediately without requiring sweep
- On pickup: suppress MIDI until crossover, then immediately send the current physical value so the DAW snaps to match

**Pickup scope**
- Full pickup FSM (MIDI suppression until crossover) applies to: sliders (faders), pan pots, and all other assignable knobs after a bank switch
- Not visual-only — all three control types suppress output until they sync

**State LEDs (loop, punch, metronome)**
- All three state LEDs are in scope: Loop (Cycle), Punch In/Out, Metronome (Click)
- Physical buttons are on the grid (K1–K16)
- Button-to-function mapping lives in a compile-time config header (e.g., `mcuConfig.h`) — not EEPROM, not hardcoded in logic
- LED responds immediately (instant snap) when Logic sends Note On/Off for that state — no fade
- Brightness: same as channel strip LEDs (LED_MAX_BRIGHTNESS)

### Claude's Discretion
- Exact config header name and structure for button mapping
- Which specific grid LEDs align to which rows for the sync indicators (researcher should verify against pinDefines.h and physical layout)
- ADC noise filtering for crossover detection (3-value threshold already exists on Potentiometer class — may be sufficient)
- How pickup FSM state is stored (per-control struct, bitfield, etc.)

### Deferred Ideas (OUT OF SCOPE)
- None — discussion stayed within phase scope.
</user_constraints>

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| LED-03 | Loop active, punch in/out, and metronome toggle states reflected in grid button LEDs | Logic Pro sends Note On/Off for these states; NoteRegistry already provides note→MCUButton* lookup; need to know the correct note numbers and wire the correct grid buttons to them |
| PICK-01 | Each fader tracks last known DAW value received via incoming pitch bend on MIDI channels 1–8 | Requires registering a `usbMIDI.setHandlePitchBend()` callback in main.cpp; Fader class needs a `_dawValue14bit` field and a `setDawValue()` method; value arrives as signed -8192..+8191, store as 0..16383 after +8192 offset |
| PICK-02 | Fader MIDI output is suppressed until physical position crosses through (or within tolerance of) the DAW value | FSM states: SYNCED / OUT_OF_SYNC; `Fader::read()` checks FSM state before calling `sendPitchBend`; deadband ±2–3 MIDI units on the 14-bit scale (±32–48 in 14-bit equivalent of 7-bit ±2–3) |
| PICK-03 | Channel button LED blinks (variable rate based on distance from DAW value) while fader is out of sync | MCUButton::setLedState(1) stub is already in place; implement blink using millis()-based timer in Fader or InputManager; pulse rate encodes distance — faster when far, slower as physical approaches |
| PICK-04 | LED blink stops and fader becomes active exactly when pickup occurs | On crossover: set FSM to SYNCED, call P-button MCUButton to stop blink, immediately send current physical value to DAW |
| PICK-05 | All 8 faders re-enter pickup mode (blink) on every bank switch | Requires detecting bank switch event from Logic; bank switch is signaled by Logic sending a burst of pitch bend updates on all 8 channels simultaneously (or via specific MCU SysEx); on detection, call `enterPickupMode()` on all 8 faders — but per CONTEXT.md, visual blink is lazy-revealed on interaction, not immediate |
| PICK-06 | Boundary edge case handled: faders at 0 or 127 pick up immediately if DAW value matches | In crossover check: if physical == 0 and DAW == 0, or physical == 127 and DAW == 127 (boundary equivalents in 14-bit), trigger pickup without sweep |
</phase_requirements>

---

## Summary

Phase 2 has two parallel tracks: (1) the pickup mode FSM for faders (and by user decision, pan pots and knobs too), and (2) wiring the loop/punch/metronome grid button LEDs to Logic Pro's Note On/Off feedback. Both tracks build directly on top of the Phase 1 foundation with no new external libraries.

The pickup FSM is the more complex track. Each fader needs three things: a stored DAW value (received via incoming pitch bend), a blink timer for the channel strip LED, and suppression logic that blocks `sendPitchBend` until the physical position enters a ±deadband around the DAW value. The blink is triggered lazily — only when the user first interacts with that channel strip after a bank switch. Bank switches are detected by incoming pitch bend bursts from Logic that update all 8 fader DAW values simultaneously.

The LED-03 track (loop/punch/metronome) is simpler: Logic Pro sends Note On/Off for these state changes. The NoteRegistry already routes note→MCUButton* and setLedState() already handles velocity 127 (on) and 0 (off). The only work is determining the correct MCU note numbers, wiring the correct grid button MCUButton instances to those notes in the NoteRegistry, and creating a compile-time config header that documents (and controls) the mapping.

**Primary recommendation:** Implement the two tracks as separate plans. Track 1 (PICK-01 through PICK-06): extend Fader with DAW value tracking and FSM; extend MCUButton with millis()-based blink; add pitch bend receive callback in main.cpp; detect bank switch. Track 2 (LED-03): identify MCU note numbers for Loop/Punch/Metronome, add a `mcuConfig.h` compile-time mapping header, register grid buttons in NoteRegistry for those notes.

---

## Standard Stack

### Core (no changes from Phase 1)

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Teensyduino usbMIDI | Built into Teensy core | Pitch bend receive (`setHandlePitchBend`), existing NoteOn/Off routing | Native; no alternatives |
| SoftPWM (vendored) | 1.0.1-local | LED blink via repeated SoftPWMSet calls from millis()-gated loop | Already in use; SOFTPWM_MAXCHANNELS=22 confirmed |
| Bounce2 | ^2.70 | Button debounce (unchanged from Phase 1) | Already wired |

### No New Libraries Required

Phase 2 is pure firmware logic. Everything needed exists in the Phase 1 stack:
- Blink via millis() timing — no timer library needed; SoftPWM handles PWM; the loop() calls the blink update each iteration
- Pickup FSM — struct with enum state, no state machine library needed at this scale
- DAW value tracking — add one `int` field to Fader

**No new platformio.ini lib_deps entries are needed for Phase 2.**

---

## Architecture Patterns

### Recommended File Changes After Phase 2

```
src/
├── main.cpp            # Add handlePitchBend callback; register setHandlePitchBend; add bank switch detection
├── pinDefines.h        # Unchanged
├── mcuConfig.h         # NEW: compile-time button-to-function mapping (Loop note, Punch note, Metronome note, grid button indices)
├── mcuProtocol.h/cpp   # Unchanged
├── mcuButton.h/cpp     # Add blink state: _blinking bool, _blinkPeriodMs uint16_t, _lastBlinkMs uint32_t; updateBlink() method
├── fader.h/cpp         # Add _dawValue14bit int, _pickupState enum, setDawValue(), enterPickupMode(); modify read() to suppress when OUT_OF_SYNC
├── noteRegistry.h/cpp  # Unchanged (already supports note→MCUButton* lookup)
├── inputManager.h/cpp  # Add updateBlinks() called from loop; add onBankSwitch() to put all faders in pickup mode; handle incoming pitch bend routing
├── potentiometer.h/cpp # Consider adding pickup FSM for pan pots (CONTEXT.md scope); or defer pan pot pickup to separate plan
└── button.h/cpp        # Dead code — unchanged
```

### Pattern 1: Fader Pickup FSM

**What:** A two-state machine per fader — SYNCED (normal MIDI output) and OUT_OF_SYNC (MIDI suppressed, blink active on associated channel button). Transitions are driven by crossover detection in `Fader::read()` and by the bank switch event.

**State transitions:**
```
SYNCED → OUT_OF_SYNC   : onBankSwitch() called (triggered by bank change detection in main.cpp)
OUT_OF_SYNC → SYNCED   : crossover detected in read() — physical enters deadband around _dawValue14bit
```

**MIDI suppression:** In `Fader::read()`, only call `usbMIDI.sendPitchBend()` when `_pickupState == SYNCED`. When OUT_OF_SYNC, update `_lastFader14bit` (to track physical position for crossover check) but do not send.

**Crossover detection in Fader::read():**
```cpp
// Source: CONTEXT.md decisions — deadband ±2-3 MIDI units on 7-bit = ±16-24 on 14-bit range
// Deadband: ±PICKUP_DEADBAND (16-24 in 14-bit) around _dawValue14bit
if (_pickupState == OUT_OF_SYNC) {
    // Boundary edge case (PICK-06): both at rail
    bool bothAtZero    = (fader14bit <= PICKUP_DEADBAND && _dawValue14bit <= PICKUP_DEADBAND);
    bool bothAtMax     = (fader14bit >= 16383 - PICKUP_DEADBAND && _dawValue14bit >= 16383 - PICKUP_DEADBAND);
    bool inDeadband    = abs(fader14bit - _dawValue14bit) <= PICKUP_DEADBAND;

    if (bothAtZero || bothAtMax || inDeadband) {
        _pickupState = SYNCED;
        if (_channelBtn != nullptr) _channelBtn->stopBlink();
        // Immediately send current position so DAW snaps
        usbMIDI.sendPitchBend(fader14bit - 8192, _midiChannel);
        _lastFader14bit = fader14bit;
        return;
    }
    // Still out of sync — update position for next crossover check but suppress MIDI
    _lastFader14bit = fader14bit;
    return;
}
```

**Lazy reveal:** The blink on the channel strip button is NOT activated when `onBankSwitch()` is called. Instead, it is activated the first time the user interacts with that channel strip (fader move, button press, or pan pot move) while the fader is in OUT_OF_SYNC state. `Fader::read()` detects the first movement and calls `_channelBtn->startBlink()` at that point.

**Channel-button association:** Each Fader needs a pointer to its corresponding track button (MCUButton* for P1–P8). This pointer is set during `InputManager::init()` via a new `Fader::setChannelButton(MCUButton* btn)` method. The association is: fader[0] (channel 1) ↔ trackButtons[0] (P1), ..., fader[7] (channel 8) ↔ trackButtons[7] (P8).

**Example Fader extension:**
```cpp
// In fader.h — additions for Phase 2
enum PickupState { SYNCED, OUT_OF_SYNC };

class Fader {
public:
    // ...existing...
    void setDawValue(int value14bit);   // called from handlePitchBend callback
    void enterPickupMode();             // called on bank switch; sets OUT_OF_SYNC, resets blink-reveal flag
    void setChannelButton(MCUButton* btn);  // wired in InputManager::init()

    static const int PICKUP_DEADBAND = 24;  // ±24 in 14-bit ≈ ±3 in 7-bit MIDI

private:
    int         _dawValue14bit = -1;  // -1 = no DAW value received yet
    PickupState _pickupState   = SYNCED;
    bool        _blinkRevealed = false;  // lazy reveal: true after first interaction in OUT_OF_SYNC
    MCUButton*  _channelBtn    = nullptr;
};
```

### Pattern 2: MCUButton Millis-Based Blink

**What:** A non-blocking LED blink implemented in MCUButton using `millis()` timing. The blink period is variable — shorter (faster) when the fader is far from the DAW value, longer (slower) as it approaches. `startBlink(period)` and `stopBlink()` control the state; `updateBlink()` must be called every loop iteration.

**Implementation:**
```cpp
// In mcuButton.h — additions for Phase 2
class MCUButton {
public:
    // ...existing...
    void startBlink(uint16_t periodMs);  // activates blink with given period; half-period = on, half = off
    void stopBlink();                    // stops blink; sets LED to off (per CONTEXT.md: "go dark immediately")
    void updateBlink();                  // call every loop(); no-op if not blinking
    bool isBlinking() const;

private:
    bool     _blinking      = false;
    uint16_t _blinkPeriodMs = 500;
    uint32_t _lastBlinkMs   = 0;
    bool     _blinkPhase    = false;  // false=off, true=on
};

// In mcuButton.cpp
void MCUButton::updateBlink() {
    if (!_blinking || !_hasLed) return;
    uint32_t now = millis();
    uint32_t halfPeriod = _blinkPeriodMs / 2;
    if (now - _lastBlinkMs >= halfPeriod) {
        _blinkPhase = !_blinkPhase;
        SoftPWMSet(_ledPin, _blinkPhase ? LED_MAX_BRIGHTNESS : 0);
        _lastBlinkMs = now;
    }
}

void MCUButton::stopBlink() {
    _blinking   = false;
    _blinkPhase = false;
    if (_hasLed) SoftPWMSet(_ledPin, 0);  // go dark immediately — no confirmation flash
}
```

**Blink period derivation (distance encoding):**
```cpp
// Called by Fader when first interaction detected in OUT_OF_SYNC state
// distance = abs(physical14bit - dawValue14bit), range 0..16383
// Map distance to period: far = fast (200ms), close = slow (600ms)
uint16_t computeBlinkPeriod(int distance14bit) {
    // Clamp to 0..16383, map to 200..600ms (faster when farther)
    int clamped = min(distance14bit, 16383);
    return (uint16_t)map(clamped, 0, 16383, 600, 200);
}
```

Note: The period should be refreshed each time `Fader::read()` detects a significant physical position change while OUT_OF_SYNC, so the "radar ping" effect dynamically updates as the user moves the fader.

### Pattern 3: Incoming Pitch Bend Receive (PICK-01)

**What:** Register `usbMIDI.setHandlePitchBend()` in main.cpp to receive fader position updates from Logic Pro. Logic sends pitch bend on MIDI channels 1–8 to tell the controller where each fader currently is. This is how the controller learns the DAW value for each fader.

**Teensyduino pitch bend receive signature:**
```cpp
// Source: https://www.pjrc.com/teensy/td_midi.html (official PJRC API docs)
// Teensyduino callback signature for pitch bend receive:
void handlePitchBend(byte channel, int value) {
    // channel: 1-8 (MIDI channel, maps to fader index channel-1)
    // value: -8192 to +8191 (Teensyduino signed format)
    // Convert to 14-bit: value + 8192 → 0..16383
    if (channel >= 1 && channel <= 8) {
        int value14bit = value + 8192;
        inputManager.setFaderDawValue(channel - 1, value14bit);
    }
}

// Register in setup():
usbMIDI.setHandlePitchBend(handlePitchBend);
```

**InputManager routing:**
```cpp
// In inputManager.h/cpp — new method
void InputManager::setFaderDawValue(int faderIndex, int value14bit) {
    if (faderIndex < 0 || faderIndex >= NUM_TRACKS) return;
    trackFaders[faderIndex].setDawValue(value14bit);
}
```

### Pattern 4: Bank Switch Detection (PICK-05)

**What:** Logic Pro signals a bank switch by sending pitch bend updates on all 8 MIDI channels in rapid succession (typically within one USB MIDI packet cycle). The controller detects this pattern and calls `enterPickupMode()` on all 8 faders.

**Detection approach:** Count how many faders receive a new DAW value within a short window (e.g., 20ms). If all 8 update within the window, treat it as a bank switch.

**Alternative simpler approach (recommended):** Treat every incoming pitch bend as a potential bank-switch trigger. If a fader's DAW value changes AND the fader is currently SYNCED AND the difference between the old and new DAW value exceeds a threshold (e.g., >256 in 14-bit, which is >2 in 7-bit), the fader enters OUT_OF_SYNC. This avoids the complexity of a time-window counter.

```cpp
// In Fader::setDawValue() — simplified bank switch detection
void Fader::setDawValue(int newValue14bit) {
    if (_dawValue14bit >= 0 && _pickupState == SYNCED) {
        int delta = abs(newValue14bit - _dawValue14bit);
        if (delta > BANK_SWITCH_THRESHOLD) {  // e.g., 256 in 14-bit
            enterPickupMode();
        }
    }
    _dawValue14bit = newValue14bit;
}
```

This is simpler, per-fader, and requires no global coordination. The constant `BANK_SWITCH_THRESHOLD` can be tuned if normal fader automation creates false positives.

**IMPORTANT: Per CONTEXT.md, the blink is NOT started immediately on bank switch.** `enterPickupMode()` sets `_pickupState = OUT_OF_SYNC` and resets `_blinkRevealed = false`, but does not call `startBlink()`. The blink is lazily revealed on first user interaction.

### Pattern 5: LED-03 — Loop/Punch/Metronome State LEDs

**What:** Logic Pro sends Note On/Off messages for transport state toggles. The NoteRegistry already maps note → MCUButton*. The only remaining work is knowing the correct note numbers and wiring the right grid button MCUButton instances to those notes.

**MCU note numbers for mode buttons (MEDIUM confidence — must verify with hardware MIDI monitor):**

Based on community MCU documentation (TouchMCU, libMackieControl), the approximate note numbers are:

| Function | Note Number | Confidence |
|----------|------------|------------|
| Loop/Cycle | 86 | MEDIUM — multiple sources agree |
| Punch In (start) | 85 | MEDIUM |
| Punch Out (end) | 86 | LOW — some sources merge punch with cycle |
| Metronome/Click | 89 | MEDIUM |
| Zoom | 100 | LOW |

**CRITICAL NOTE:** These note numbers have LOW-MEDIUM confidence from community sources. The STATE.md Blockers section explicitly flags "Phase 3 note numbers (LOW confidence): Mode button note assignments in the 69–75 range (CYCLE, CLICK, etc.) use approximate values in some sources." The researcher found differing values across sources (some place CYCLE at 86, some at 69, some at 71). **This must be verified with a MIDI monitor against Logic Pro before the note numbers are hardcoded.** The config header approach (Claude's discretion decision) makes this easy to change.

**Compile-time config header approach:**
```cpp
// src/mcuConfig.h — NEW FILE
// Compile-time mapping of MCU note numbers to grid button positions
// Change these if Logic Pro sends different notes (verify with MIDI monitor)

#ifndef mcu_config_h
#define mcu_config_h

// Grid button indices (0-based) for mode functions
// K1=index 0 ... K16=index 15
#define MCU_LOOP_GRID_INDEX     8   // K9 — verify against physical layout
#define MCU_PUNCH_GRID_INDEX    9   // K10 — verify
#define MCU_METRO_GRID_INDEX    10  // K11 — verify

// MCU note numbers for mode state LEDs (Logic Pro sends these to update LED state)
// MUST verify with MIDI monitor — community sources differ
#define MCU_NOTE_LOOP           86
#define MCU_NOTE_PUNCH_IN       85
#define MCU_NOTE_METRONOME      89

#endif
```

**NoteRegistry wiring (in InputManager::init()):**
```cpp
// Add to InputManager::init() after existing noteRegistry.registerButton() calls:
noteRegistry.registerButton(MCU_NOTE_LOOP,       &gridButtons[MCU_LOOP_GRID_INDEX]);
noteRegistry.registerButton(MCU_NOTE_PUNCH_IN,   &gridButtons[MCU_PUNCH_GRID_INDEX]);
noteRegistry.registerButton(MCU_NOTE_METRONOME,  &gridButtons[MCU_METRO_GRID_INDEX]);
```

Since `setLedState()` already handles velocity 127 (on) and 0 (off) with LED_MAX_BRIGHTNESS, and since CONTEXT.md says "instant snap, no fade" — the existing `setLedState()` behavior already satisfies LED-03. No changes to MCUButton::setLedState() are needed for LED-03.

### Pattern 6: Grid LED Sync Indicators (CONTEXT.md Visual Feedback Beyond REQUIREMENTS.md)

**What:** CONTEXT.md adds grid LED visual feedback for pan pot and slider sync state, beyond what REQUIREMENTS.md specifies for PICK-03. This is a confirmed user decision. Grid rows map to sync indicators:
- Grid row 1 (K1–K4, only 4 LEDs but 8 channels): one LED per channel for pan pot sync state — but hardware has 4 LEDs in row 1, not 8. Resolution: columns K1–K4 may represent channels 1–4 and some from row 2 represent 5–8, OR 8 LEDs across rows 1–2 are used. This needs planner clarification.
- Grid rows 2–3 (K5–K8, K9–K12, 8 LEDs total): one LED per slider for slider sync state

**Physical layout from pinDefines.h and inputManager.cpp:**
```
Row 0 (top):    K1  K2  K3  K4   (indices 0-3,  y=0)
Row 1:          K5  K6  K7  K8   (indices 4-7,  y=1)
Row 2:          K9  K10 K11 K12  (indices 8-11, y=2)
Row 3 (bottom): K13 K14 K15 K16  (indices 12-15, y=3)
```

CONTEXT.md says "Grid row 1: one LED per channel indicates pan pot sync state" and "Grid rows 2–3: one LED per slider indicates slider sync state (8 LEDs total, physically aligned to horizontal position of each slider)."

The grid has 4 LEDs per row. 8 channels cannot map 1:1 to a single 4-LED row. Interpretation options:
1. Row 1 (K5–K8, 4 LEDs) = channels 1–4 pan pot sync; Row 2 (K9–K12, 4 LEDs) = channels 5–8 pan pot sync — this gives 8 total and uses rows 1–2 for pan pots, rows 2–3 for sliders (overlap on row 2 → conflict)
2. Row 1 (K1–K4) and row 2 (K5–K8) combined = 8 LEDs for pan pots (using "row 1" loosely meaning top portion); rows 2–3 (K9–K12, which is actually a single physical row) — still only 4 LEDs, not 8

**This is an open question that the planner must resolve before implementation.** The researcher recommends treating rows as 0-indexed (K1–K4 = row 0, K5–K8 = row 1) and interpreting CONTEXT.md "row 1" as K5–K8 (4 LEDs) + K9–K12 (4 LEDs) for 8 pan pot indicators total, and "rows 2–3" as meaning K9–K16 minus transport buttons. However, this overlaps with LED-03 buttons and transport. The planner needs explicit grid layout resolution before implementing the grid sync indicators.

### Anti-Patterns to Avoid

- **Blocking millis() calls:** Never use `delay()` for blink timing. Use `millis()` delta gating. The loop() runs at ~1kHz; delay() in any blink handler would break MIDI processing.
- **Starting blink immediately on bank switch:** CONTEXT.md explicitly says reveal is lazy. Do not call `startBlink()` in `enterPickupMode()`.
- **Sending MIDI in OUT_OF_SYNC state:** The entire point of pickup mode is MIDI suppression. `Fader::read()` must guard on `_pickupState == SYNCED` before calling `usbMIDI.sendPitchBend()`.
- **Not sending a snap value on pickup:** When crossover fires, PICK-04 requires immediately sending the physical value so the DAW snaps to match. Without this, the DAW stays at the old position and the controller is SYNCED but disagreeing with DAW.
- **Hardcoding mode note numbers in inputManager.cpp:** CONTEXT.md decision mandates a compile-time config header. Put note numbers in `mcuConfig.h`, not inline in init().
- **Calling setLedState() for mode LEDs with any fade:** CONTEXT.md says "instant snap." The existing `SoftPWMSetFadeTime(ledPin, 125, 125)` set in `MCUButton::init()` means SoftPWM will fade. For LED-03 mode buttons, either accept the 125ms fade (fast enough to feel instant) or call `SoftPWMSetFadeTime(ledPin, 0, 0)` for those specific buttons. 125ms is likely acceptable.
- **Ignoring NoteOff for mode LED off:** Logic may send either NoteOn(vel=0) or NoteOff for LED-off. The existing Phase 1 dual callback (handleNoteOn + handleNoteOff) already handles both cases. No change needed.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Non-blocking timing | delay()-based blink | millis()-delta gating (already used in RippleState) | SoftPWM handles PWM; millis() handles time gating with zero CPU block |
| Blink period calculation | Complex curve function | Arduino map() from distance to ms range | Sufficient for linear interpolation; non-linear feel can be approximated with two map calls |
| Note lookup for mode buttons | Inline note number constants | mcuConfig.h compile-time defines | Keeps note numbers in one place; easy to update after hardware MIDI monitor verification |
| State machine library | External FSM framework | Simple enum + switch in Fader class | FSM has 2 states; a library adds unnecessary complexity |

**Key insight:** The entire Phase 2 implementation is pure C++ struct/enum logic wired to existing SoftPWM and usbMIDI APIs. There is nothing here that requires an external library or custom protocol implementation.

---

## Common Pitfalls

### Pitfall 1: Deadband Units (14-bit vs 7-bit confusion)

**What goes wrong:** Applying a ±3 MIDI unit deadband directly to the 14-bit range. A ±3 unit deadband in 7-bit MIDI (0–127) maps to ±~384 units in 14-bit (0–16383): `3 / 127 * 16383 ≈ 387`. But the fader physical ADC is 10-bit (0–1023) mapped to 14-bit. ADC noise on the 10-bit range is ~3 counts (the Potentiometer ANALOG_NOISE), which maps to ~48 in 14-bit (the FADER_NOISE_THRESHOLD). The pickup deadband should be larger than the fader noise threshold (48) to avoid false triggers — recommend PICKUP_DEADBAND = 128 to 256 in 14-bit, which is approximately ±1 to ±2 in 7-bit MIDI. Start at 128; tune if needed.

**Why it happens:** Confusing the three ranges: 7-bit MIDI (0–127), 10-bit ADC (0–1023), 14-bit MCU fader (0–16383).

**How to avoid:** Do all pickup math in 14-bit. The constant PICKUP_DEADBAND should be defined in 14-bit units. Derive it as approximately 2–3x the FADER_NOISE_THRESHOLD (48) → 128 as a good starting point.

**Warning signs:** Fader picks up immediately without requiring a sweep (deadband too large), or requires exact position match (deadband too small → ADC noise prevents pickup).

### Pitfall 2: Blink and setLedState() Conflict

**What goes wrong:** Logic Pro may send Note On messages to a channel strip button (P1–P8) at any time (e.g., arming a track for record). If the channel button is currently blinking due to pickup mode, the incoming `setLedState(velocity)` call from the NoteRegistry will directly call `SoftPWMSet()` and conflict with the blink timer's alternating state.

**Why it happens:** Two independent code paths (blink timer, NoteRegistry callback) both call `SoftPWMSet()` on the same LED pin with no coordination.

**How to avoid:** In `MCUButton::setLedState()`, if `velocity == 127`, stop the blink (call `stopBlink()`) and then set the LED solid. If `velocity == 0`, stop the blink and set LED off. Only `velocity == 1` (the MCU "blink" velocity from Logic) should allow blinking. This means pickup-mode blink is always overridden by actual DAW LED state — which is the correct behavior (Logic's LED state takes priority over pickup feedback).

**Warning signs:** Channel strip LED flickers erratically when Logic sends LED updates during fader pickup mode.

### Pitfall 3: Pitch Bend Receive Callback Not Registered

**What goes wrong:** PICK-01 requires `usbMIDI.setHandlePitchBend()` but main.cpp currently has no pitch bend receive handler. If it's not registered, Logic's fader position feedback silently goes nowhere and DAW values are never learned.

**Why it happens:** Phase 1 did not need to receive pitch bend — faders only sent it. Receiving requires explicit registration.

**How to avoid:** Add `usbMIDI.setHandlePitchBend(handlePitchBend)` in `setup()` alongside the existing NoteOn/NoteOff/SysEx registrations.

**Warning signs:** All 8 faders remain in OUT_OF_SYNC forever even after moving them to the correct position (no DAW value was ever learned so `_dawValue14bit` stays -1).

### Pitfall 4: Fader _dawValue14bit Uninitialized on First Bank Switch

**What goes wrong:** If the controller powers on and immediately switches banks before any pitch bend is received from Logic, `_dawValue14bit` is -1 (sentinel). Calling `setDawValue()` with a -1 sentinel will skip the bank-switch delta check. The fader won't enter pickup mode for the first bank switch.

**How to avoid:** In the bank-switch delta detection within `setDawValue()`, guard on `_dawValue14bit >= 0` before computing the delta. On first DAW value receipt after power-on, set `_dawValue14bit` without entering pickup mode. This is acceptable behavior — the controller starts SYNCED at boot.

**Warning signs:** PICK-05 doesn't work on the very first bank switch after power-on.

### Pitfall 5: MCU Note Numbers for Loop/Punch/Metronome Are Not Confirmed

**What goes wrong:** Hardcoding the wrong note numbers causes LED-03 to silently fail (the NoteRegistry lookup succeeds for the wrong button, or returns null for the right button). Logic Pro sends the mode note but the wrong grid button LED updates, or none does.

**Why it happens:** Community MCU documentation has conflicting note numbers for mode buttons. STATE.md explicitly flags this as LOW confidence.

**How to avoid:** Before implementing LED-03, use a MIDI monitor (or add temporary Serial.print in handleNoteOn) to log all incoming Note messages while toggling Loop/Punch/Metronome in Logic. Document the actual note numbers, then set them in `mcuConfig.h`.

**Warning signs:** Toggling Loop in Logic doesn't change any grid LED, or changes the wrong LED.

### Pitfall 6: updateBlink() Not Called in loop()

**What goes wrong:** Adding `startBlink()` to MCUButton but forgetting to call `updateBlink()` in the main loop means the blink state machine never advances and LEDs stay static.

**Why it happens:** The blink update must be called every loop iteration for time-based gating to work. There is no interrupt or background task on this embedded system.

**How to avoid:** Add a `updateBlinks()` method to InputManager that iterates all trackButtons and calls `updateBlink()` on each. Call `inputManager.updateBlinks()` from `loop()` in main.cpp alongside `readAll()`.

---

## Code Examples

### Incoming Pitch Bend Registration (Phase 2 addition to main.cpp)

```cpp
// Source: https://www.pjrc.com/teensy/td_midi.html (official PJRC API)
// Teensyduino pitch bend receive — value is -8192..+8191
void handlePitchBend(byte channel, int value) {
    if (channel >= 1 && channel <= 8) {
        inputManager.setFaderDawValue(channel - 1, value + 8192);
    }
}

// In setup() — add after existing handler registrations:
usbMIDI.setHandlePitchBend(handlePitchBend);
```

### Fader FSM State Check in read()

```cpp
// In Fader::read() — replace existing unconditional sendPitchBend with FSM-guarded version
void Fader::read() {
    if (_pin < 0) return;

    int raw       = analogRead(_pin);
    int fader14bit = map(raw, 0, 1023, 0, 16383);

    // Always update position for crossover tracking (even when suppressed)
    bool significantMove = (_lastFader14bit < 0 ||
                            abs(fader14bit - _lastFader14bit) > FADER_NOISE_THRESHOLD);
    if (!significantMove) return;

    _lastFader14bit = fader14bit;

    if (_pickupState == OUT_OF_SYNC) {
        // Lazy reveal: first significant move while OUT_OF_SYNC triggers blink
        if (!_blinkRevealed && _channelBtn != nullptr) {
            int distance = abs(fader14bit - _dawValue14bit);
            _channelBtn->startBlink(computeBlinkPeriod(distance));
            _blinkRevealed = true;
        } else if (_blinkRevealed && _channelBtn != nullptr) {
            // Refresh blink period as position changes
            int distance = abs(fader14bit - _dawValue14bit);
            _channelBtn->startBlink(computeBlinkPeriod(distance));
        }

        // Check crossover
        bool bothAtZero = (fader14bit <= PICKUP_DEADBAND && _dawValue14bit >= 0 && _dawValue14bit <= PICKUP_DEADBAND);
        bool bothAtMax  = (fader14bit >= 16383 - PICKUP_DEADBAND && _dawValue14bit >= 16383 - PICKUP_DEADBAND);
        bool inDeadband = (_dawValue14bit >= 0 && abs(fader14bit - _dawValue14bit) <= PICKUP_DEADBAND);

        if (bothAtZero || bothAtMax || inDeadband) {
            _pickupState = SYNCED;
            if (_channelBtn != nullptr) _channelBtn->stopBlink();
            // PICK-04: immediately send snap value
            usbMIDI.sendPitchBend(fader14bit - 8192, _midiChannel);
        }
        return;  // suppress MIDI while OUT_OF_SYNC
    }

    // SYNCED: normal MIDI output
    usbMIDI.sendPitchBend(fader14bit - 8192, _midiChannel);
}
```

### MCUButton Blink Update (loop integration)

```cpp
// In InputManager::updateBlinks() — new method, called from loop()
void InputManager::updateBlinks() {
    for (int i = 0; i < NUM_TRACKS; i++) {
        trackButtons[i].updateBlink();
    }
}

// In main.cpp loop() — add after existing calls:
if (mcuProtocol.isHandshakeComplete()) {
    inputManager.readAll();
    inputManager.updateBlinks();  // Phase 2: drive LED blink state machines
} else {
    inputManager.readIdle();
    inputManager.updateRipple();
}
```

---

## Open Questions

1. **Grid sync indicator layout — 8 channels across 4-LED rows**
   - What we know: CONTEXT.md says "Grid row 1: one LED per channel indicates pan pot sync state" and "Grid rows 2–3: one LED per slider indicates slider sync state (8 LEDs total)." The grid has 4 LEDs per row. 8 channels cannot fit in one 4-LED row.
   - What's unclear: Which specific grid LEDs (K1–K12) map to which channel for pan pot sync indicators? The CONTEXT.md "rows" may refer to physical grid rows (4 LEDs each) or a conceptual grouping. Rows 2–3 could mean the physical rows y=1 and y=2 (K5–K12), giving 8 LEDs total — which fits 8 channels exactly for sliders. Row 1 alone has only 4 LEDs for 8 channels — possible resolution: two rows (y=0 and y=1 = K1–K8) serve as pan pot indicators.
   - Recommendation: Planner should resolve this before the grid sync indicator task. Suggest: K1–K8 (rows 0–1) = 8 pan pot indicators (Ch1–Ch8), K9–K16 (rows 2–3) = potential slider indicators — but K13–K16 are Shift/Record/Stop/Play. This means only K9–K12 (4 LEDs) are free for sliders. Either only 4 sliders get indicators, or the mapping is different. **This needs explicit resolution by the user or planner before implementation.**

2. **MCU note numbers for Loop, Punch In/Out, Metronome — must verify with hardware**
   - What we know: Community sources suggest approximately: Loop=86, Punch In=85, Metronome=89. STATE.md flags these as LOW confidence.
   - What's unclear: Logic Pro's exact note numbers for these mode toggles.
   - Recommendation: First task of the LED-03 plan should be a "verify note numbers" step using a MIDI monitor or temporary Serial logging of all incoming Note messages. Gate the LED wiring on confirmed note numbers.

3. **Pan pot pickup scope — Potentiometer class extension**
   - What we know: CONTEXT.md says pickup mode applies to pan pots (Potentiometer/knobs), not just faders. The Potentiometer class currently sends CC unconditionally. Extending it with the same FSM would require a `_dawValue` field and suppression logic.
   - What's unclear: How does Logic Pro send feedback for pan pot (VPot) positions? The MCU protocol sends VPot ring CC (48–55) for VPot display, but this was deferred to v2 in REQUIREMENTS.md (VPT-01). Without incoming VPot feedback from Logic, the controller has no DAW value to sync against for pan pots.
   - Recommendation: If Logic doesn't send VPot ring CC to the controller, pan pot pickup mode is impossible (no reference value). This is likely an open question requiring hardware testing. May need to descope pan pot pickup from Phase 2 if no feedback path exists. Planner should flag this explicitly.

4. **Bank switch detection reliability**
   - What we know: The per-fader delta approach (`setDawValue()` detects large delta → `enterPickupMode()`) is simpler than time-window counting.
   - What's unclear: Whether Logic Pro's fader automation sends continuous pitch bend updates during playback that could trigger false positives (large delta during normal automation).
   - Recommendation: Use a larger BANK_SWITCH_THRESHOLD (e.g., 512 in 14-bit = ~4 in 7-bit) to avoid false triggers from automation. Tune with hardware testing.

---

## Validation Architecture

> Skipped — `workflow.nyquist_validation` is not set to `true` in `.planning/config.json`. Testing for this project is manual and hardware-in-loop (per REQUIREMENTS.md "Out of Scope: Unit tests / test framework").

Validation for Phase 2 is manual hardware testing against Logic Pro:

1. **PICK-01:** Open MIDI monitor. Move Logic on-screen fader. Confirm `_dawValue14bit` is being set (add temporary Serial.print if needed).
2. **PICK-02:** After bank switch, move physical fader. Confirm MIDI output is suppressed (MIDI monitor shows no pitch bend until crossover).
3. **PICK-03:** Channel strip button LED blinks after first physical interaction post-bank-switch. Blink rate visibly changes as fader approaches target.
4. **PICK-04:** Blink stops exactly at crossover. MIDI monitor shows one pitch bend sent immediately at pickup.
5. **PICK-05:** Switching banks in Logic triggers all 8 faders to enter OUT_OF_SYNC (confirmed by interacting with each channel strip).
6. **PICK-06:** Move Logic fader to 0. Move all physical faders to 0. All faders pick up immediately without requiring sweep.
7. **LED-03:** Toggle Loop/Cycle in Logic. Corresponding grid button LED lights and goes dark with Logic's state. Same for Punch and Metronome.

---

## Sources

### Primary (HIGH confidence)
- `/Users/kenmarut/repo/phaedr-midi-controller/src/fader.h` — Existing Fader class; FADER_NOISE_THRESHOLD = 48; `_lastFader14bit` pattern; sendPitchBend with -8192 offset confirmed working in Phase 1 hardware verification
- `/Users/kenmarut/repo/phaedr-midi-controller/src/mcuButton.h/cpp` — Existing MCUButton class; `setLedState()` stub with velocity==1 comment "handled Phase 2"; SoftPWMSet API usage; LED_MAX_BRIGHTNESS from pinDefines.h
- `/Users/kenmarut/repo/phaedr-midi-controller/src/inputManager.cpp` — RippleState pattern using millis()-gated timing (RIPPLE_HOLD_MS, `millis() - startMs`); established pattern for non-blocking time-based LED control
- `/Users/kenmarut/repo/phaedr-midi-controller/src/main.cpp` — Existing handler registration pattern; `usbMIDI.setHandleNoteOn/Off/SysEx`; Phase 2 must add `usbMIDI.setHandlePitchBend`
- `https://www.pjrc.com/teensy/td_midi.html` — Official PJRC API: `setHandlePitchBend(void (*)(byte channel, int value))` confirmed; pitch bend receive signature
- `.planning/phases/02-led-state-pickup-mode/02-CONTEXT.md` — User decisions: all locked choices documented above

### Secondary (MEDIUM confidence)
- `.planning/phases/01-mcu-protocol-foundation/01-RESEARCH.md` — Phase 1 research on MCU note numbers (transport confirmed working); NoteRegistry pattern verified in hardware
- `.planning/phases/01-mcu-protocol-foundation/01-05-SUMMARY.md` — Hardware verification: NoteRegistry routing, setLedState() LED_MAX_BRIGHTNESS confirmed; Phase 2 readiness stated
- `https://github.com/Do-sth-sharp/libMackieControl/blob/main/doc/MackieControl.md` — MCU mode button note numbers (Loop, Punch, Metronome approximate values)
- `https://github.com/NicoG60/TouchMCU/blob/main/doc/mackie_control_protocol.md` — Additional mode note number reference

### Tertiary (LOW confidence — verify with hardware MIDI monitor before use)
- MCU note numbers for Loop (86?), Punch In (85?), Metronome (89?) — community sources differ; STATE.md explicitly flags as LOW confidence
- Whether Logic Pro sends VPot ring CC (48–55) back to the controller during pan pot moves — required for pan pot pickup mode but unverified

---

## Metadata

**Confidence breakdown:**
- Fader pickup FSM (PICK-01 through PICK-06): HIGH — all APIs verified in Phase 1 hardware; pattern is straightforward C++ struct extension; no new territory
- MCUButton blink (PICK-03): HIGH — millis() pattern already established in RippleState; SoftPWM API confirmed working for all 22 LEDs
- LED-03 wiring (NoteRegistry): HIGH — NoteRegistry routing is verified and working for P1–P8 and transport buttons
- LED-03 note numbers: LOW — community sources conflict; must verify with hardware MIDI monitor
- Pan pot pickup: LOW — requires Logic to send VPot feedback; no confirmed feedback path exists; may need descoping
- Grid sync indicator layout: MEDIUM — requires planner resolution of 4-LED-per-row vs 8-channel mapping ambiguity
- Bank switch detection: MEDIUM — per-fader delta approach is sensible but threshold needs hardware tuning

**Research date:** 2026-02-25
**Valid until:** 2026-08-25 (Teensyduino API stable; MCU protocol stable; Logic Pro MCU behavior stable across versions)
