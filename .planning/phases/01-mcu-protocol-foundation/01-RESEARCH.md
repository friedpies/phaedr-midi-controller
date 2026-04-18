# Phase 1: MCU Protocol Foundation - Research

**Researched:** 2026-02-21
**Domain:** Mackie Control Universal (MCU) MIDI protocol on Teensy 3.5 / Teensyduino
**Confidence:** MEDIUM-HIGH (protocol well-documented by community; Logic Pro behavior cross-verified via multiple sources; Teensyduino API verified against official PJRC source)

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

- Device name: **"Phaedr"** — this is what Logic Pro displays in its Control Surfaces panel
- SysEx serial bytes: ASCII "PHAEDR" + null pad — `0x50 0x48 0x41 0x45 0x44 0x52 0x00`
- These 7 bytes appear in the Host Connection Query and confirm messages
- **Wait for Logic** — all LEDs with LED hardware update only after Logic sends a Note On/Off confirmation; no optimistic local toggling
- Play (btn 16) and Stop (btn 15) have no LEDs — the wait-for-Logic rule is irrelevant for them
- Record (btn 14) has an LED — waits for Logic to confirm record arm state
- Channel strip buttons (REC/SOLO/MUTE/SELECT) all wait for Logic
- The Button class must NOT toggle LED state on press in MCU mode
- **Clean break** — all CC output removed. Firmware speaks MCU only after Phase 1
- The existing CC-based ButtonRegistry can be replaced entirely by a Note-to-Button lookup
- No multi-protocol support, no compile-time mode switch
- **New MCUButton class** — handles MCU-mode buttons (sends Note Bang, no local LED toggle, LED updates only via `setLedState()` from DAW feedback)
- **New Fader class** — handles MCU-mode sliders (sends 14-bit Pitch Bend on per-fader MIDI channel, not CC); separate from the Potentiometer class which stays for knobs
- Existing `Button` and `Potentiometer` classes can remain as dead code or be removed — Claude's discretion on cleanup
- Existing `ButtonRegistry` can be replaced or extended — Claude's discretion, given MCU-only direction
- **Pure MCU Note Bang**: send Note On followed immediately by Note Off; don't track toggle state locally
- Logic Pro owns all channel button state; hardware just sends bangs and displays whatever Logic says
- If Logic doesn't respond to the handshake — Claude's discretion on retry strategy (retry periodically is preferred over silent wait indefinitely)

### Claude's Discretion

- Whether to keep or delete `Button` and `Potentiometer` classes after new MCU classes are introduced
- ButtonRegistry replacement architecture (new class vs. extend existing)
- Pre-handshake retry interval/count
- Debug Serial output removal strategy (define vs. delete)

### Deferred Ideas (OUT OF SCOPE)

- **Pre-handshake idle LED animation** — belongs in Phase 4 (Animation Manager), not Phase 1. Phase 4 should implement a pre-connection idle pattern (e.g., slow pulse or sweep) that plays until the MCU handshake completes.
</user_constraints>

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| FIX-01 | Fix copy-by-value Button bug in `inputManager.cpp:25` — use pointer or reference so DAW-driven `setLedState()` calls actually update physical LEDs | Identified in existing code: `Button button = *(buttonRegistry.ccNumToButton[ccNum])` creates a stack copy; fix is `Button* button = buttonRegistry.ccNumToButton[ccNum]; button->setLedState(...)` |
| FIX-02 | Remove `Serial.println("CONTROL CHANGE")` from MIDI callbacks — debug output causes timing jitter at 24 PPQN clock rates | Identified in `main.cpp:29`; also `handleStart()` and `handleClock()` have Serial.println — all must be removed |
| MCU-01 | Firmware completes the 4-step MCU SysEx handshake with Logic Pro within 300ms | Research provides exact byte sequences and challenge-response algorithm; see SysEx Handshake section |
| MCU-02 | Logic Pro recognizes the controller as a Mackie Control Universal surface | Achieved when handshake completes; requires `usbMIDI.setHandleSystemExclusive()` callback + correct response bytes |
| MCU-03 | Channel strip buttons (REC, SOLO, MUTE, SELECT) send MCU Note Bang messages on MIDI channel 1 using note numbers 0–31 | Note map verified: REC=0-7, SOLO=8-15, MUTE=16-23, SELECT=24-31; MCUButton class sends NoteOn(127)+NoteOff(0) immediately |
| MCU-04 | Transport buttons (Rewind, FF, Stop, Play, Record) send MCU Note Bang messages using note numbers 91–95 | Note numbers verified: Rewind=91, FF=92, Stop=93, Play=94, Record=95 |
| MCU-05 | Sliders (faders) send 14-bit pitch bend on MIDI channels 1–8, not CC messages | Fader class sends `usbMIDI.sendPitchBend(value - 8192, channel)` where value is 0–16383 mapped from ADC |
| MCU-06 | Knobs send CC 16–23 (VPot encoder messages) per MCU protocol | Potentiometer class needs CC numbers changed: knob 1→CC16, knob 8→CC23; relative format: 0x41=CW+1, 0x01=CCW+1 |
| LED-01 | Channel strip REC/SOLO/MUTE/SELECT LEDs update in response to Note On/Off messages from Logic (velocity 127=on, 1=blink, 0=off) | `usbMIDI.setHandleNoteOn()` callback routes note→LED via NoteRegistry; velocity 127=solid, 1=blink (Phase 2 blink detail), 0=off |
| LED-02 | Transport state LEDs (Play, Record) reflect real Logic Pro state via incoming Note On/Off | Same NoteRegistry lookup; note 94=Play LED, note 95=Record LED |
| LED-04 | ButtonRegistry extended with Note-to-Button lookup alongside existing CC-to-Button map | Replace or extend ButtonRegistry with `std::map<int, MCUButton*> noteToButton`; existing CC map becomes dead code with clean-break decision |
</phase_requirements>

---

## Summary

This phase transforms the firmware from a custom CC-based controller into a Mackie Control Universal (MCU) surface that Logic Pro natively recognizes. The work has three distinct pillars: (1) SysEx handshake to get Logic to register the device, (2) replacing CC output with MCU Note Bang messages for buttons and Pitch Bend for faders, and (3) wiring bidirectional LED feedback through a Note-indexed registry.

The MCU protocol is well-documented by the community through open-source implementations (Ardour, TouchMCU, Control Surface library). The core handshake involves a 4-message SysEx exchange with a challenge-response computation. The note number map for channel strip and transport buttons is stable across all sources. The critical Teensyduino pitfall is that `sendPitchBend` uses a signed -8192 to +8191 range, while MCU specifies 0–16383, requiring an offset of -8192 in all fader math.

The biggest prerequisite is FIX-01 — the copy-by-value bug in `inputManager.cpp:25` silently discards all DAW-driven LED updates and must be the very first change. FIX-02 (Serial debug output) must also be eliminated before MIDI clock timing can be trusted. Everything else builds on a clean foundation.

**Primary recommendation:** Fix FIX-01 and FIX-02 first. Then implement handshake, then MCUButton + Fader classes, then NoteRegistry. Test each step with a MIDI monitor before proceeding to the next.

---

## Standard Stack

### Core (already in project)

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Teensyduino usbMIDI | Built into Teensy core | USB MIDI send/receive including SysEx | Native Teensy USB MIDI; no additional library needed for MCU |
| Bounce2 | ^2.70 | Button debouncing | Already in use; keep for MCUButton |
| SoftPWM | ^1.0.1 | PWM LED fading | Already in use; keep for LED brightness control |

### No New Libraries Required

The entire MCU protocol implementation uses only the built-in `usbMIDI` object from Teensyduino. The SysEx handshake, Note Bang sends, and Pitch Bend fader messages are all standard Teensyduino calls.

**No new platformio.ini lib_deps entries are needed for Phase 1.**

---

## Architecture Patterns

### Recommended File Structure After Phase 1

```
src/
├── main.cpp            # Register new SysEx + NoteOn/Off handlers; keep loop() pattern
├── pinDefines.h        # Unchanged
├── mcuProtocol.h/cpp   # SysEx handshake state machine; sends/receives handshake messages
├── mcuButton.h/cpp     # NEW: Note Bang send, LED update via setLedState() only
├── fader.h/cpp         # NEW: Pitch Bend send on per-fader channel; replaces trackSliders
├── noteRegistry.h/cpp  # NEW or extended ButtonRegistry: note→MCUButton* map
├── inputManager.h/cpp  # Updated: uses MCUButton[] and Fader[] instead of Button[]/Potentiometer[] for buttons/sliders
├── potentiometer.h/cpp # Kept: used for knobs (CC 16–23); update CC numbers only
├── button.h/cpp        # Kept as dead code OR deleted — Claude's discretion
└── buttonRegistry.h/cpp # Replaced by noteRegistry OR extended — Claude's discretion
```

### Pattern 1: MCU SysEx Handshake State Machine

**What:** A 4-step SysEx exchange that Logic Pro initiates when a device appears on a MIDI port configured as Mackie Control. The device must respond correctly within ~300ms per message or Logic declares the surface unavailable.

**When to use:** Triggered by `usbMIDI.setHandleSystemExclusive()` callback. The state machine tracks whether handshake has completed and retries the Device Query response on a configurable timer if Logic doesn't proceed.

**Handshake flow (Logic Pro sends first):**

```
Step 1: Logic → Device (Device Query)
  F0 00 00 66 14 00 F7

Step 2: Device → Logic (Host Connection Query — device identifies itself)
  F0 00 00 66 14 01 [serial: 7 bytes] [challenge: 4 bytes] F7
  Serial = 0x50 0x48 0x41 0x45 0x44 0x52 0x00  ("PHAEDR\0")
  Challenge = any 4 non-zero 7-bit bytes (e.g., 0x7A 0x6B 0x5C 0x4D)

Step 3: Logic → Device (Host Connection Reply — Logic answers challenge)
  F0 00 00 66 14 02 [serial: 7 bytes] [response: 4 bytes] F7

Step 4: Device → Logic (Confirmation — device accepts Logic's response)
  F0 00 00 66 14 03 [serial: 7 bytes] F7
```

**Challenge-response algorithm** (device validates Logic's answer in Step 3):

```cpp
// Source: Ardour surface.cc + TouchMCU protocol docs (cross-verified)
// c[] = the 4 challenge bytes the device sent in Step 2
// r[] = the 4 response bytes Logic returns in Step 3
// Device must verify: r[i] == expected[i]
uint8_t expected[4];
expected[0] = 0x7F & (c[0] + (c[1] ^ 0x0A) - c[3]);
expected[1] = 0x7F & ((c[2] >> 4) ^ (c[0] + c[3]));
expected[2] = 0x7F & ((c[3] - (c[2] << 2)) ^ (c[0] | c[1]));
expected[3] = 0x7F & (c[1] - c[2] + (0xF0 ^ (c[3] << 4)));
```

**Important:** The challenge-response is purely cosmetic authentication. Some DIY implementations skip validation and always send Confirmation (Step 4) regardless of Logic's response. This is acceptable for a single-device setup. Recommended approach: validate the response and send Confirmation if it matches; if validation fails, do NOT confirm (Logic will retry).

**Example implementation skeleton:**

```cpp
// Source: Ardour surface.cc algorithm, Teensy forum patterns
const uint8_t MCU_HEADER[] = {0xF0, 0x00, 0x00, 0x66, 0x14};
const uint8_t SERIAL[7]    = {0x50, 0x48, 0x41, 0x45, 0x44, 0x52, 0x00}; // "PHAEDR\0"
uint8_t _challenge[4]      = {0x7A, 0x6B, 0x5C, 0x4D}; // static; could be random

void handleSysEx(const uint8_t *data, uint16_t length, bool complete) {
    // Check header: F0 00 00 66 14
    if (length < 7) return;
    if (memcmp(data, MCU_HEADER, 5) != 0) return;

    uint8_t msgType = data[5];
    if (msgType == 0x00) {
        // Step 1: Device Query received — send Host Connection Query (Step 2)
        uint8_t msg[18];
        msg[0] = 0xF0; msg[1] = 0x00; msg[2] = 0x00; msg[3] = 0x66; msg[4] = 0x14;
        msg[5] = 0x01;
        memcpy(msg + 6, SERIAL, 7);
        memcpy(msg + 13, _challenge, 4);
        msg[17] = 0xF7;
        usbMIDI.sendSysEx(18, msg, true); // hasTerm=true: data already includes F0/F7
    } else if (msgType == 0x02) {
        // Step 3: Host Connection Reply — validate and send Confirmation (Step 4)
        const uint8_t *r = data + 13; // Logic's 4 response bytes
        uint8_t expected[4];
        expected[0] = 0x7F & (_challenge[0] + (_challenge[1] ^ 0x0A) - _challenge[3]);
        expected[1] = 0x7F & ((_challenge[2] >> 4) ^ (_challenge[0] + _challenge[3]));
        expected[2] = 0x7F & ((_challenge[3] - (_challenge[2] << 2)) ^ (_challenge[0] | _challenge[1]));
        expected[3] = 0x7F & (_challenge[1] - _challenge[2] + (0xF0 ^ (_challenge[3] << 4)));
        if (memcmp(r, expected, 4) == 0) {
            uint8_t confirm[13];
            confirm[0] = 0xF0; confirm[1] = 0x00; confirm[2] = 0x00;
            confirm[3] = 0x66; confirm[4] = 0x14; confirm[5] = 0x03;
            memcpy(confirm + 6, SERIAL, 7);
            confirm[12] = 0xF7;
            usbMIDI.sendSysEx(13, confirm, true);
            _handshakeComplete = true;
        }
    }
}

// In setup():
usbMIDI.setHandleSystemExclusive(handleSysEx); // 3-arg version
```

**sendSysEx hasTerm parameter:** When `hasTerm=true`, the data array already contains F0 and F7; the library does not add them again. When `hasTerm=false`, the library wraps the data in F0/F7 automatically and `length` excludes the terminators.

### Pattern 2: MCUButton — Note Bang Without Local State

**What:** Sends Note On (velocity 127) immediately followed by Note Off (velocity 0) on the same note. No local LED toggle. LED state is only updated via `setLedState()` called from the NoteOn/Off callback.

**Note number assignments (MIDI channel 1 for all):**

```
REC (Record Ready):  Ch1=0,  Ch2=1,  Ch3=2,  Ch4=3,  Ch5=4,  Ch6=5,  Ch7=6,  Ch8=7
SOLO:                Ch1=8,  Ch2=9,  Ch3=10, Ch4=11, Ch5=12, Ch6=13, Ch7=14, Ch8=15
MUTE:                Ch1=16, Ch2=17, Ch3=18, Ch4=19, Ch5=20, Ch6=21, Ch7=22, Ch8=23
SELECT:              Ch1=24, Ch2=25, Ch3=26, Ch4=27, Ch5=28, Ch6=29, Ch7=30, Ch8=31

Transport:
  Rewind=91, FastForward=92, Stop=93, Play=94, Record=95
```

**Example:**

```cpp
// Source: libMackieControl docs (verified against midibox protocol mappings)
void MCUButton::read() {
    _bounce.update();
    if (_bounce.fell()) {
        usbMIDI.sendNoteOn(_noteNum, 127, 1);  // channel 1, velocity 127
        usbMIDI.sendNoteOff(_noteNum, 0, 1);   // immediate Note Off
    }
    // No LED toggle here — LED is updated only via setLedState()
}

void MCUButton::setLedState(uint8_t velocity) {
    // velocity: 0=off, 1=blink (Phase 2), 127=on
    if (velocity == 0) {
        SoftPWMSet(_ledPin, 0);
    } else if (velocity == 127) {
        SoftPWMSet(_ledPin, 255);
    }
    // velocity==1 (blink) handled in Phase 2
}
```

### Pattern 3: Fader — 14-bit Pitch Bend on Per-Channel MIDI

**What:** Reads analog fader position, maps to 14-bit range 0–16383, converts to Teensyduino's signed pitch bend range (-8192 to +8191), and sends on MIDI channel matching fader number (fader 1 → channel 1, fader 8 → channel 8).

**Critical offset:** Teensyduino `sendPitchBend` uses signed -8192 to +8191. MCU fader bottom=0 maps to -8192, top=16383 maps to +8191. Formula: `sendPitchBend(rawValue - 8192, channel)`.

**Example:**

```cpp
// Source: PJRC usb_midi.h (verified), libMackieControl fader spec
void Fader::read() {
    int raw = analogRead(_pin);  // 0–1023 (10-bit default) or 0–4095 (12-bit)
    // Map ADC range to 14-bit MCU fader range
    int fader14bit = map(raw, 0, 1023, 0, 16383);  // adjust max if using 12-bit ADC
    if (abs(fader14bit - _lastFader14bit) > FADER_NOISE_THRESHOLD) {
        _lastFader14bit = fader14bit;
        int pitchBendValue = fader14bit - 8192;  // convert to signed Teensyduino range
        usbMIDI.sendPitchBend(pitchBendValue, _midiChannel);  // channel 1–8
    }
}
```

### Pattern 4: NoteRegistry — Note-to-Button Lookup

**What:** Replaces or wraps `ButtonRegistry`. Maps note number → `MCUButton*` so incoming Note On/Off from Logic can reach the correct hardware button's LED.

**Example:**

```cpp
// New class or replacement for ButtonRegistry
class NoteRegistry {
public:
    std::map<uint8_t, MCUButton*> noteToButton;

    void registerButton(uint8_t noteNum, MCUButton* btn) {
        noteToButton[noteNum] = btn;
    }
};

// In main.cpp callback:
void handleNoteOn(byte channel, byte note, byte velocity) {
    auto it = noteRegistry.noteToButton.find(note);
    if (it != noteRegistry.noteToButton.end()) {
        it->second->setLedState(velocity);  // velocity 127=on, 1=blink, 0=off
    }
}
void handleNoteOff(byte channel, byte note, byte velocity) {
    // NoteOff with velocity 0 = LED off
    auto it = noteRegistry.noteToButton.find(note);
    if (it != noteRegistry.noteToButton.end()) {
        it->second->setLedState(0);
    }
}

// In setup():
usbMIDI.setHandleNoteOn(handleNoteOn);
usbMIDI.setHandleNoteOff(handleNoteOff);
```

### Pattern 5: VPot Knob CC Messages

**What:** The knobs are physical potentiometers (not encoders), but MCU-06 requires them to send CC 16–23. The existing `Potentiometer` class already handles analog read and CC send — only the CC numbers need updating.

**MCU VPot CC format:** The existing `sendControlChange(cc, value, 1)` is correct for knob absolute position values. True VPot encoders use a relative format (0x41=CW+1, 0x01=CCW+1), but since the hardware has analog pots (not encoders), sending absolute 0–127 on CC 16–23 is the correct approach — Logic Pro will interpret them as VPot absolute position.

**CC number reassignment:**

```
KNOB_1 → CC 16  (was CC 14)
KNOB_2 → CC 17  (was CC 15)
KNOB_3 → CC 18  (was CC 28)
KNOB_4 → CC 19  (was CC 29)
KNOB_5 → CC 20  (was CC 30)
KNOB_6 → CC 21  (was CC 31)
KNOB_7 → CC 22  (was CC 118)
KNOB_8 → CC 23  (was CC 119)
```

### Anti-Patterns to Avoid

- **Optimistic LED toggle on press:** The Button class currently does `ledState = !ledState` on fell(). MCUButton must NOT do this. Logic Pro sends the LED state; the hardware only reflects it.
- **Copy-by-value button dereference:** `Button button = *(ptr)` makes a copy. Always use `Button* button = ptr` and call `button->setLedState()`.
- **Serial.println in MIDI callbacks:** Especially in handleClock() which fires 24 times per beat. Even a single `println` can stall MIDI processing long enough to miss clock pulses.
- **Sending SysEx without F7:** Always include F7 as the last byte when `hasTerm=true`, or ensure `hasTerm=false` and let the library add it.
- **Using wrong MIDI channel for faders:** Fader 1 → channel 1, Fader 8 → channel 8. Channel 9 is the MCU master fader (not needed in Phase 1).

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Button debouncing | Custom debounce timer | Bounce2 (already in project) | Handles contact bounce, configurable interval, fell()/rose() API |
| PWM LED control | manual analogWrite timing | SoftPWM (already in project) | Already wired to LED pins, handles fade timing |
| SysEx parsing | State machine from scratch | Read `data[5]` for message type and dispatch | SysEx arrives as complete buffer in the 3-arg callback |
| Note-to-button map | Array linear search | `std::map<uint8_t, MCUButton*>` | Already used by ButtonRegistry; O(log n) lookup |

**Key insight:** The MCU protocol itself is the hard part — the Teensyduino API (`usbMIDI`) handles all USB MIDI framing. There is no need for additional MIDI libraries.

---

## Common Pitfalls

### Pitfall 1: FIX-01 — Copy-by-Value Button Bug (CRITICAL, must fix first)

**What goes wrong:** `Button button = *(buttonRegistry.ccNumToButton[ccNum])` in `inputManager.cpp:25` creates a temporary stack copy of the Button object. Calling `button.setLedState()` updates the copy's LED state but not the original. The physical LED never changes.

**Why it happens:** C++ value semantics — dereferencing a pointer with `*` then assigning to a non-reference creates a copy.

**How to avoid:** Use a pointer: `Button* button = buttonRegistry.ccNumToButton[ccNum]; button->setLedState(velocity == 127 ? HIGH : LOW);`

**Warning signs:** LED never responds to DAW input even though MIDI traffic is confirmed in a monitor.

### Pitfall 2: FIX-02 — Serial Output in MIDI Callbacks (CRITICAL)

**What goes wrong:** `Serial.println("CONTROL CHANGE")` in the CC callback, `Serial.println("HANDLE START")` in handleStart(), and `Serial.println("HANDLE CLOCK")` in handleClock() block the MIDI processing loop. At 120 BPM, 24 PPQN = 48 clock messages/second. Each `println` may take 1–10ms, causing missed clocks and eventual desync.

**Why it happens:** Serial output is synchronous; the USB MIDI loop in `loop()` can't process the next message until `println` completes.

**How to avoid:** Delete all `Serial.println` calls from MIDI callbacks. If debug output is needed later, use a compile-time `#define DEBUG` guard.

**Warning signs:** Beat chaser (Phase 4) timing is erratic; MIDI clock jitter visible in DAW.

### Pitfall 3: Pitch Bend Signed/Unsigned Mismatch

**What goes wrong:** Sending `usbMIDI.sendPitchBend(fader14bit, channel)` where `fader14bit` is 0–16383 causes clipping: values above 8191 are clamped to 8191 inside the library (it checks `if (value > 8191) value = 8191`).

**Why it happens:** Teensyduino's sendPitchBend expects -8192 to +8191 (signed 14-bit centered at 0), but MCU spec uses 0–16383 (unsigned 14-bit centered at 8192).

**How to avoid:** Always apply offset: `usbMIDI.sendPitchBend(fader14bit - 8192, channel)`

**Warning signs:** Logic faders only move in the lower half; pushing physical fader to max results in fader at center on screen.

### Pitfall 4: SysEx Callback Signature Mismatch

**What goes wrong:** Registering a 2-argument SysEx callback `void mySysEx(byte *data, unsigned int size)` works for small messages but has a fixed buffer limit. The MCU Host Connection Reply is 18 bytes, which should fit, but the 3-argument version is safer and preferred.

**How to avoid:** Always use the 3-argument form: `void handleSysEx(const uint8_t *data, uint16_t length, bool complete)` and register with `usbMIDI.setHandleSystemExclusive(handleSysEx)`.

**Warning signs:** SysEx callback never fires, or fires with truncated data.

### Pitfall 5: Logic Pro Control Surfaces Setup — Must Add Manually

**What goes wrong:** Logic does NOT auto-detect MCU surfaces on arbitrary MIDI ports. The user must go to Logic Pro → Preferences (or Settings in newer versions) → Control Surfaces → Setup → New → Mackie Control, then assign the correct MIDI in/out ports.

**Why it matters:** Even if the firmware is perfect, Logic will not send the Device Query until the surface is configured. This is an expected setup step, not a bug.

**Warning signs:** No SysEx ever arrives from Logic; MIDI monitor shows no traffic from Logic's direction.

### Pitfall 6: SysEx hasTerm Parameter Confusion

**What goes wrong:** Calling `usbMIDI.sendSysEx(length, data, false)` when `data` already includes F0 and F7 causes double-wrapping: the library adds F0/F7 around the data that already has them, corrupting the message.

**How to avoid:** If your byte array starts with `0xF0` and ends with `0xF7`, use `hasTerm=true`. If your array is just the content without terminators, use `hasTerm=false`.

### Pitfall 7: gridButtons[] and trackButtons[] Are Stored by Value in InputManager

**What goes wrong:** `Button gridButtons[NUM_GRID_BUTTONS]` stores Button objects by value in the array. When switching to MCUButton, the same pattern applies: `MCUButton mcuButtons[N]` is fine because `readAll()` iterates the array directly and calls methods on the actual objects (no pointer indirection needed here). The bug from FIX-01 was specifically in the CC handler where the registry returns a pointer that gets dereferenced into a copy.

**Why it matters:** The NoteRegistry must store `MCUButton*` pointers to elements of the array, not copies. These pointers remain valid for the lifetime of InputManager (which is global). Do NOT take the address of a temporary.

---

## Code Examples

Verified patterns from official sources:

### Teensyduino sendNoteOn / sendNoteOff (Note Bang)

```cpp
// Source: https://www.pjrc.com/teensy/td_midi.html (official PJRC docs)
// Send Note Bang: NoteOn then immediate NoteOff
usbMIDI.sendNoteOn(noteNumber, 127, 1);   // note, velocity=127 (pressed), channel=1
usbMIDI.sendNoteOff(noteNumber, 0, 1);   // note, velocity=0 (released), channel=1
```

### Teensyduino sendPitchBend for Faders

```cpp
// Source: https://github.com/PaulStoffregen/cores/blob/master/teensy3/usb_midi.h
// sendPitchBend(int value, uint8_t channel) — value range: -8192 to +8191
// MCU fader 14-bit raw = 0 to 16383; offset by 8192 to convert to signed
int faderMCU = map(analogRead(pin), 0, 1023, 0, 16383);
usbMIDI.sendPitchBend(faderMCU - 8192, midiChannel);  // midiChannel 1-8
```

### Teensyduino sendSysEx

```cpp
// Source: https://github.com/PaulStoffregen/cores/blob/master/teensy3/usb_midi.h
// hasTerm=true: data array includes F0 start and F7 end bytes
// hasTerm=false: library wraps data in F0/F7 automatically
uint8_t confirmMsg[] = {0xF0, 0x00, 0x00, 0x66, 0x14, 0x03,
                         0x50, 0x48, 0x41, 0x45, 0x44, 0x52, 0x00,
                         0xF7};
usbMIDI.sendSysEx(14, confirmMsg, true);  // hasTerm=true
```

### Teensyduino SysEx Receive Callback (3-arg form)

```cpp
// Source: https://www.pjrc.com/teensy/td_midi.html (official PJRC docs)
// 3-argument version handles messages larger than internal buffer via multiple calls
void handleSysEx(const uint8_t *data, uint16_t length, bool complete) {
    // 'complete' is true when this is the final (or only) chunk
    if (!complete) return;  // wait for full message
    // process data[0..length-1]
}
// Register in setup():
usbMIDI.setHandleSystemExclusive(handleSysEx);
```

### NoteOn Callback Registration

```cpp
// Source: https://www.pjrc.com/teensy/td_midi.html
void handleNoteOn(byte channel, byte note, byte velocity) {
    // Called when Logic Pro sends Note On (e.g., LED state updates)
}
void handleNoteOff(byte channel, byte note, byte velocity) {
    // Called when Logic Pro sends Note Off
}
// Register in setup():
usbMIDI.setHandleNoteOn(handleNoteOn);
usbMIDI.setHandleNoteOff(handleNoteOff);
```

### MCU Challenge-Response Algorithm

```cpp
// Source: Ardour surface.cc (open source, verified)
// https://github.com/ardour/ardour/blob/master/libs/surfaces/mackie/surface.cc
// c = 4 challenge bytes sent by device; r = 4 response bytes received from Logic
// Returns true if Logic's response is valid
bool validateChallengeResponse(const uint8_t c[4], const uint8_t r[4]) {
    uint8_t expected[4];
    expected[0] = 0x7F & (c[0] + (c[1] ^ 0x0A) - c[3]);
    expected[1] = 0x7F & ((c[2] >> 4) ^ (c[0] + c[3]));
    expected[2] = 0x7F & ((c[3] - (c[2] << 2)) ^ (c[0] | c[1]));
    expected[3] = 0x7F & (c[1] - c[2] + (0xF0 ^ (c[3] << 4)));
    return memcmp(expected, r, 4) == 0;
}
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `usbMIDI.sendPitchBend(0–16383)` unsigned | `sendPitchBend(-8192 to +8191)` signed | Teensyduino 1.41 | All fader math needs -8192 offset |
| 2-arg SysEx callback | 3-arg callback with chunking | Teensyduino 1.x | 3-arg preferred; handles large SysEx safely |
| CC-based button output | Note Bang (NoteOn+NoteOff) | This phase | Logic Pro MCU protocol requires Note messages for channel strip/transport |

**Deprecated in this project after Phase 1:**
- `usbMIDI.sendControlChange` for buttons (replaced by Note Bang)
- `usbMIDI.sendControlChange` for sliders (replaced by Pitch Bend)
- `usbMIDI.setHandleControlChange` as the LED feedback handler (replaced by setHandleNoteOn/NoteOff)
- `ButtonRegistry.ccNumToButton` map (replaced by note-indexed lookup)

---

## Open Questions

1. **Does Logic Pro 10.8+ use a different SysEx model ID byte (0x14 vs 0x10)?**
   - What we know: Model ID 0x14 is Mackie Control Universal; 0x10 is Logic Control (older). Ardour checks for both. Community implementations targeting Logic Pro use 0x14.
   - What's unclear: Whether Logic Pro 10.8+ (current) responds differently to 0x14 vs 0x10.
   - Recommendation: Use 0x14 (Mackie Control Universal). If Logic does not respond, test with 0x10. Verify with MIDI monitor on first hardware test.

2. **Pre-handshake retry interval**
   - What we know: Logic sends the Device Query only when a surface is configured. If Logic restarts or the Teensy power-cycles after Logic is already running, Logic may not resend the query.
   - What's unclear: Whether the device can proactively trigger re-discovery.
   - Recommendation: Send an unsolicited Host Connection Query (message type 0x01) every 5 seconds until handshake completes. Some DIY implementations report this works to wake Logic.

3. **Does Logic send LED feedback as NoteOn or NoteOff for off-state?**
   - What we know: MCU spec defines: velocity 127 = on, 1 = blink, 0 = off. Some sources indicate Logic sends NoteOff (not NoteOn with velocity 0) for LED-off.
   - What's unclear: Whether Logic uses NoteOn(vel=0) or NoteOff for the off state.
   - Recommendation: Register BOTH `setHandleNoteOn` AND `setHandleNoteOff`. In NoteOn handler, velocity=0 should also turn LED off (MIDI convention treats NoteOn+vel0 as NoteOff). In NoteOff handler, always turn LED off.

4. **Fader noise threshold for 14-bit range**
   - What we know: Current Potentiometer uses `ANALOG_NOISE = 3` on a 0–1023 ADC scale (10-bit). After mapping to 0–16383, the equivalent noise threshold is ~48.
   - What's unclear: Whether the existing ADC noise level causes excessive fader "chatter" after 16x scaling.
   - Recommendation: Start with a threshold of 48 (proportionally equivalent to current 3). If Logic fader displays jitter, increase to 64 or 96.

---

## Sources

### Primary (HIGH confidence)

- `https://github.com/PaulStoffregen/cores/blob/master/teensy3/usb_midi.h` — Verified sendPitchBend signed range (-8192 to +8191 with internal clamping), sendSysEx hasTerm parameter, 3-arg SysEx callback signature
- `https://www.pjrc.com/teensy/td_midi.html` — Official PJRC Teensyduino MIDI API reference; sendNoteOn/Off, sendSysEx, setHandleNoteOn/Off, setHandleSystemExclusive
- `https://github.com/ardour/ardour/blob/master/libs/surfaces/mackie/surface.cc` — Open source DAW MCU implementation; challenge-response algorithm (calculate_challenge_response function); verified against community docs

### Secondary (MEDIUM confidence)

- `https://github.com/Do-sth-sharp/libMackieControl/blob/main/doc/MackieControl.md` — Channel strip note number assignments (REC=0-7, SOLO=8-15, MUTE=16-23, SELECT=24-31), transport (Rewind=91, FF=92, Stop=93, Play=94, Record=95), fader MIDI channel assignments; cross-verified with midibox mappings
- `https://github.com/NicoG60/TouchMCU/blob/main/doc/mackie_control_protocol.md` — LED velocity semantics (0=off, 1=blink, 127=on); SysEx handshake message structure
- `http://www.midibox.org/dokuwiki/doku.php?id=mc_protocol_mappings` — MCU note number cross-reference table confirming channel strip assignments
- `https://forum.pjrc.com/threads/50036-Sonar-Cakewalk-Mackie-SysEx` — Teensy-specific SysEx send/receive patterns; serial number format
- VPot CC encoding (CC 16–23, relative format 0x41=CW, 0x01=CCW) — multiple community sources agree

### Tertiary (LOW confidence — validate against Logic Pro on hardware)

- Logic Pro-specific SysEx behavior (model ID 0x14, retry behavior) — documented from community reverse engineering, not from official Apple spec
- Whether Logic Pro 10.8+ sends NoteOn(vel=0) vs NoteOff for LED-off state — conflicting community reports

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — Teensyduino API verified against official PJRC source code; no new libraries needed
- MCU note numbers: MEDIUM-HIGH — consistent across 3+ community sources; no official Mackie spec available
- SysEx handshake bytes: MEDIUM — community-documented via reverse engineering; cross-verified in Ardour source
- Challenge-response algorithm: HIGH — code verified in open-source Ardour codebase
- Logic Pro behavior: MEDIUM — community sources; must validate on hardware with MIDI monitor
- Pitfalls: HIGH — FIX-01 and FIX-02 directly observed in existing source code

**Research date:** 2026-02-21
**Valid until:** 2026-08-21 (MCU protocol is stable; Teensyduino API rarely changes; Logic Pro SysEx behavior stable for years)
