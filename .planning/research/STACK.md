# Stack Research

**Domain:** Teensy 3.5 USB MIDI controller firmware — Mackie Control Universal (MCU) protocol + LED animation
**Researched:** 2026-02-21
**Confidence:** MEDIUM-HIGH (protocol constants from authoritative community docs; Teensy API from official PJRC docs)

---

## Recommended Stack

### Core Technologies

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| Teensy usbMIDI (built-in) | Teensyduino core | Send/receive all MIDI including SysEx | Already in use; handles Note, CC, PitchBend, SysEx, and RealTime (Clock) natively. No additional library needed for MCU protocol messages. |
| Arduino framework | Current (via PlatformIO) | Hardware abstraction, setup()/loop() | Already in use. MCU protocol is implemented entirely as MIDI message handling in loop(), no framework change required. |
| PlatformIO | Current | Build system | Already in use. No changes to build toolchain needed. |
| Bounce2 | ^2.70 | Button debouncing | Already in use. Keep as-is. |
| SoftPWM | ^1.0.1 | LED brightness/fading | Already in use. Compatible with MCU additions. Uses PIT/IntervalTimer; does not conflict with usbMIDI callbacks. |

### Supporting Libraries — New Additions

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| **None required** | — | MCU protocol is pure MIDI message handling | The MCU protocol uses Note On/Off, CC, Pitch Bend, Channel Pressure, and SysEx — all handled natively by usbMIDI. No MCU-specific library is needed. |

**Decision rationale for not adding Control-Surface library:**
The Control-Surface library (tttapa) implements MCU protocol but requires adopting its entire architecture (MIDI_Interface, Control_Surface.begin(), Control_Surface.loop() pattern). Adopting it would require rewriting all existing Button, Potentiometer, InputManager, and ButtonRegistry code from scratch. For this project — which has a well-defined existing codebase and needs only MCU protocol message handling layered on top — implementing MCU messages directly with usbMIDI is simpler, more controllable, and avoids a full rewrite.

---

## MCU Protocol Message Reference

**Confidence: MEDIUM** — sourced from NicoG60/TouchMCU community documentation (most complete public MCU spec), cross-referenced with Logic Pro user documentation and Teensy forum implementations. Official Mackie protocol documentation is not publicly released.

### Faders — Pitch Bend per MIDI Channel

Each of the 8 faders uses a dedicated MIDI channel (1–8). Master fader uses channel 9.

```
DAW → Controller: usbMIDI pitch bend on channel N = fader N position
Controller → DAW: usbMIDI pitch bend on channel N = fader N moved by user
```

**Teensyduino API:**
```cpp
// Send fader position to DAW (user moved physical slider)
usbMIDI.sendPitchBend(value, channel);  // value: -8192 to +8191

// Receive fader position from DAW (pickup mode: DAW telling controller where fader is)
usbMIDI.setHandlePitchChange(myPitchChange);
void myPitchChange(byte channel, int bend) { /* bend: -8192 to +8191 */ }
```

**14-bit resolution mapping:** MCU spec uses 0–16383. Teensyduino uses -8192 to +8191. Offset by 8192 when comparing DAW value to physical ADC reading for pickup mode.

**Physical fader ADC:** 10-bit (0–1023). Map to 14-bit: `dacValue = adcValue * 16383 / 1023`. Store as -8192 to +8191 for Teensyduino: `teensyValue = (adcValue * 16383 / 1023) - 8192`.

### Channel Strip Buttons — MIDI Notes

All button messages: velocity 127 = on/pressed, velocity 0 = off/released. Channel 1.

| Function | Note Numbers (hex) | Note Numbers (decimal) | 8 channels |
|----------|-------------------|------------------------|------------|
| REC ARM | 0x00–0x07 | 0–7 | Ch 1–8 |
| SOLO | 0x08–0x0F | 8–15 | Ch 1–8 |
| MUTE | 0x10–0x17 | 16–23 | Ch 1–8 |
| SELECT | 0x18–0x1F | 24–31 | Ch 1–8 |

**Teensyduino API:**
```cpp
// Controller sends button press to DAW
usbMIDI.sendNoteOn(noteNum, 127, 1);   // press
usbMIDI.sendNoteOn(noteNum, 0, 1);    // release (Note On with vel 0 is standard)

// Controller receives LED state from DAW
usbMIDI.setHandleNoteOn(myNoteOn);
void myNoteOn(byte channel, byte note, byte velocity) {
  // velocity 127 = LED on, velocity 0 = LED off
}
```

### Transport Buttons — MIDI Notes

| Function | Note (hex) | Note (decimal) |
|----------|-----------|----------------|
| Rewind | 0x5B | 91 |
| Fast Forward | 0x5C | 92 |
| Stop | 0x5D | 93 |
| Play | 0x5E | 94 |
| Record | 0x5F | 95 |

Logic also sends Note On/Off on these to update the hardware button LEDs (e.g., Play LED lights when playing, Record LED when recording).

### Grid/Function Buttons — MIDI Notes

Additional MCU buttons the project needs to map (rows 1–3 of the grid):

| Function | Note (hex) | Note (decimal) |
|----------|-----------|----------------|
| Bank Left | 0x2E | 46 |
| Bank Right | 0x2F | 47 |
| Channel Left | 0x30 | 48 |
| Channel Right | 0x31 | 49 |
| Flip | 0x32 | 50 |
| Global View | 0x33 | 51 |
| Loop | 0x56 | 86 |
| Punch In | — | Varies by DAW mode |
| Zoom | 0x64 | 100 |
| Scrub | 0x65 | 101 |
| Up | 0x60 | 96 |
| Down | 0x61 | 97 |
| Left | 0x62 | 98 |
| Right | 0x63 | 99 |
| Click (metronome) | 0x59 | 89 |

**Note:** Full note-to-function mapping requires validation against Logic Pro's specific MCU implementation. These values are from the NicoG60/TouchMCU community documentation. **Flag for verification during Phase 1.**

### VPot Knobs — CC (Rotation) + MIDI Notes (Click)

| Function | CC Numbers | Range |
|----------|-----------|-------|
| VPot rotation | CC 16–23 (0x10–0x17), ch 1 | Relative: 1–15 = clockwise, 65–79 = counterclockwise |
| VPot click (push) | Notes 32–39 (0x20–0x27) | vel 127 = press, 0 = release |

**VPot LED Ring — CC 48–55 (0x30–0x37):**
DAW → Controller. Byte structure of the value field:
- Bits 3–0: LED position (0–11, representing LED ring positions)
- Bits 5–4: Display mode:
  - `00` = Single dot (pan/position indicator)
  - `01` = Boost/Cut (fills from center outward)
  - `10` = Wrap (fills from left up to position)
  - `11` = Spread (fills from center, both directions)
- Bit 6: Center dot LED (1 = on, 0 = off)

**This project does not have physical LED rings on knobs** — the VPot LED CC messages from Logic can be safely ignored or used only to drive animation state if a future feature needs it. No response required.

### Channel Meters — Channel Pressure (Aftertouch)

DAW → Controller. Logic sends channel pressure per MIDI channel.

Byte structure of the pressure value:
- Upper nibble (bits 6–4): channel number (0–7 for ch 1–8)
- Lower nibble (bits 3–0): meter level (0 = off, 14 = peak, 15 = clip)

```cpp
usbMIDI.setHandleAfterTouch(myAfterTouch);
void myAfterTouch(byte channel, byte pressure) {
  byte meterChannel = (pressure >> 4) & 0x07;
  byte meterLevel   = pressure & 0x0F;
}
```

**This project has no hardware meters** — this callback can be ignored.

---

## SysEx Handshake (MCU Device Query)

**Confidence: MEDIUM** — derived from NicoG60/TouchMCU documentation and corroborated by PJRC forum implementations (Sonar/Cakewalk handshake confirmed working, Logic Pro presumed equivalent).

When Logic Pro first sees an MCU device, it sends a Device Query. The controller must respond correctly or Logic may not send LED state feedback.

### Handshake Sequence (4-step)

**Step 1 — Host sends Device Query:**
```
F0 00 00 66 14 00 F7
```

**Step 2 — Controller responds with Host Connection Query (includes serial + challenge):**
```
F0 00 00 66 14 01 [7 bytes serial] [4 bytes challenge] F7
```
Total: 16 bytes. Serial can be any 7 arbitrary bytes. Challenge bytes are random (or fixed for determinism).

**Step 3 — Host replies with Host Connection Reply (computed response):**
```
F0 00 00 66 14 02 [7 bytes serial] [4 bytes response] F7
```
The 4 response bytes are computed from the challenge bytes using:
```cpp
r[0] = 0x7F & (c[0] + (c[1] ^ 0x0A) - c[3]);
r[1] = 0x7F & ((c[2] >> 4) ^ (c[0] + c[3]));
r[2] = 0x7F & ((c[3] - (c[2] << 2)) ^ (c[0] | c[1]));
r[3] = 0x7F & (c[1] - c[2] + (0xF0 ^ (c[3] << 4)));
```
(Controller receives this and verifies if desired — most implementations skip verification.)

**Step 4 — Controller sends Host Connection Confirmation:**
```
F0 00 00 66 14 03 [7 bytes serial] F7
```

### Implementation Pattern

```cpp
const uint8_t MACKIE_HEADER[] = {0xF0, 0x00, 0x00, 0x66, 0x14};
const uint8_t SERIAL_NUM[]    = {0x58, 0x59, 0x5A, 0x00, 0x00, 0x00, 0x00};

void handleSysEx(const byte* data, uint16_t length, bool complete) {
  if (!complete) return;  // Accumulate chunks if needed (MCU SysEx is short, fits in one)
  if (length < 5) return;
  if (memcmp(data, MACKIE_HEADER, 5) != 0) return;  // Not a Mackie message

  uint8_t command = data[5];

  if (command == 0x00) {
    // Device Query — respond with Host Connection Query
    uint8_t challenge[4] = {0x7F, 0x0E, 0x29, 0x43};  // fixed or random
    uint8_t response[16] = {
      0xF0, 0x00, 0x00, 0x66, 0x14, 0x01,
      SERIAL_NUM[0], SERIAL_NUM[1], SERIAL_NUM[2], SERIAL_NUM[3],
      SERIAL_NUM[4], SERIAL_NUM[5], SERIAL_NUM[6],
      challenge[0], challenge[1], challenge[2], challenge[3]
    };
    // NOTE: response is 17 bytes + F7, but F7 added by sendSysEx
    // Build and send correctly — see note below
    usbMIDI.sendSysEx(16, response, true);
  }
  else if (command == 0x02) {
    // Host Connection Reply — send Confirmation
    uint8_t confirmation[9] = {
      0xF0, 0x00, 0x00, 0x66, 0x14, 0x03,
      SERIAL_NUM[0], SERIAL_NUM[1], SERIAL_NUM[2]
      // ... extend with remaining serial bytes
    };
    // build full 13-byte confirmation message and send
  }
}

// In setup():
usbMIDI.setHandleSystemExclusive(handleSysEx);
```

**sendSysEx signature:**
```cpp
usbMIDI.sendSysEx(uint32_t length, const uint8_t* data, bool hasBeginEnd, uint8_t cable = 0);
// hasBeginEnd = true  → data includes F0...F7 bytes, length counts them
// hasBeginEnd = false → data is payload only, library adds F0/F7 automatically
```

**SysEx buffer on Teensy 3.5:** Default `USB_MIDI_SYSEX_MAX` is 290 bytes. MCU handshake messages are 14–16 bytes. No buffer size change needed.

---

## MIDI Clock — Beat Chaser Timing

**Confidence: HIGH** — MIDI Clock (24 PPQN) behavior is a formal MIDI standard. Teensy usbMIDI clock callback confirmed from official PJRC docs.

### Protocol

Logic sends MIDI Clock (status byte 0xF8) at 24 pulses per quarter note (PPQN) during playback. The existing `handleClock()` stub already receives these.

### Beat Detection Pattern

```cpp
volatile uint8_t clockCount = 0;        // 0–23 counter
volatile bool    clockRunning = false;
volatile uint8_t beatStep = 0;          // which of N grid LEDs is active

void handleStart() {
  clockRunning = true;
  clockCount   = 0;
  beatStep     = 0;
}

void handleStop() {
  clockRunning = false;
  // optionally clear beat chaser LED here
}

void handleClock() {
  if (!clockRunning) return;
  clockCount++;
  if (clockCount >= 24) {
    clockCount = 0;
    beatStep = (beatStep + 1) % GRID_LEDS;  // e.g. % 14 or % 16
    updateBeatChaserLed(beatStep);
  }
}
```

**No additional library needed.** usbMIDI already delivers clock callbacks via `setHandleClock()`. Counter modulo 24 = one beat.

### Timing Accuracy Note

The existing `loop()` approach (polling `usbMIDI.read()` at ~1000 Hz) is sufficient for beat chaser animation. MIDI clock pulses at 120 BPM arrive every ~20.8 ms; at 1000 Hz loop rate the controller reads them within ~1 ms of arrival. LED visual latency at this scale is imperceptible.

**Do not use IntervalTimer for beat chaser.** The MIDI clock itself is the timing source. IntervalTimer would decouple from actual playback tempo and drift.

---

## Pickup Mode — Algorithm

**Confidence: MEDIUM** — pattern verified from Arduino forum discussion and multiple DIY controller implementations.

### State Machine (per fader/knob)

```
States: CATCHING | SYNCED
```

```cpp
enum PickupState { CATCHING, SYNCED };

struct PotState {
  int       dacValue;      // last value sent to DAW (14-bit, -8192..+8191)
  int       dawValue;      // last value received from DAW (pitch bend)
  PickupState state;
  bool      aboveTarget;   // whether physical is above daw value when entering CATCHING
};

void updateFader(PotState& pot, int physicalRaw) {
  // Map 10-bit ADC to Teensyduino pitch bend range
  int physical = map(physicalRaw, 0, 1023, -8192, 8191);

  if (pot.state == SYNCED) {
    if (physical != pot.dacValue) {
      pot.dacValue = physical;
      usbMIDI.sendPitchBend(physical, pot.channel);
    }
  } else {
    // CATCHING: wait for physical to cross through dawValue
    bool nowAbove = (physical >= pot.dawValue);
    if (pot.aboveTarget != nowAbove) {
      // Crossed through — take control
      pot.state    = SYNCED;
      pot.dacValue = physical;
      usbMIDI.sendPitchBend(physical, pot.channel);
    }
  }
}

void onFaderReceived(byte channel, int dawValue) {
  PotState& pot = faderState[channel - 1];
  pot.dawValue  = dawValue;
  if (pot.state == SYNCED) {
    // DAW moved fader externally (bank switch, automation)
    pot.state      = CATCHING;
    pot.aboveTarget = (pot.dacValue >= dawValue);  // is physical above new target?
  }
}
```

**Channel blink during CATCHING:** Set a `blinkActive` flag when state transitions to CATCHING; clear on transition to SYNCED. Drive the channel SELECT button LED from a blink timer in `readAll()`.

---

## Alternatives Considered

| Recommended | Alternative | When to Use Alternative |
|-------------|-------------|-------------------------|
| Native usbMIDI for MCU protocol | Control-Surface library (tttapa) | Use Control-Surface if starting a project from scratch — its MCU abstraction is excellent, but retrofitting it onto existing code requires full rewrite |
| Manual beat counter (mod 24 in handleClock) | uClock library | Use uClock only if you need the Teensy to generate clock, not receive it. uClock is for clock generation with hardware timer precision, not receiving external clock. |
| Teensyduino sendPitchBend | Sending raw MIDI bytes manually | Don't; raw byte construction is error-prone and adds no benefit when usbMIDI API covers it. |
| SoftPWM for existing LEDs | Hardware PWM (analogWrite) | Hardware PWM is preferable where pins support it; SoftPWM is already established in this codebase and works fine. Only add hardware PWM if SoftPWM interrupt load causes observable issues. |

---

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| `<MIDI.h>` / `<MIDI.hpp>` for MCU messages | Already imported in main.cpp but unused effectively — Teensy's usbMIDI is the actual MIDI object. `<MIDI.h>` targets hardware serial MIDI (DIN). Including it with usbMIDI causes confusion. Remove the includes or leave unused. | `usbMIDI` object directly |
| Control-Surface library as a drop-in add | Not a drop-in; it requires owning the MIDI interface and loop. Adding it alongside existing usbMIDI.read() creates two competing MIDI event pumps. | Implement MCU messages directly in existing callback architecture |
| `delay()` in animation or clock handlers | Blocks usbMIDI.read() — MIDI messages pile up, clock ticks are missed, faders go unread | Non-blocking state machine timing using `millis()` comparisons |
| Increasing SysEx buffer beyond 290 bytes | MCU SysEx is max ~20 bytes. Increasing USB_MIDI_SYSEX_MAX requires modifying Teensyduino core headers — fragile across updates. | No change needed; default 290 bytes is sufficient |
| Hardware interrupts for MIDI clock counting | Clock arrives via USB; USB data is processed in usbMIDI.read(), not via hardware interrupt. Interrupt-based clock counting doesn't help here. | Counter in handleClock() callback |

---

## Version Compatibility

| Package | Version | Compatible With | Notes |
|---------|---------|-----------------|-------|
| Teensy usbMIDI | Teensyduino core (current) | Bounce2 ^2.70, SoftPWM ^1.0.1 | SoftPWM uses PIT/IntervalTimer; usbMIDI callbacks run in loop() context — no ISR conflict |
| Bounce2 | ^2.70 | No changes needed | Not affected by MCU additions |
| SoftPWM | ^1.0.1 | No changes needed | 30 Hz PWM update rate is adequate for button LED fading; not affected by MIDI clock callbacks |
| PlatformIO teensy35 board | Current | Arduino framework | Build flag `-DUSB_MIDI_SERIAL` enables usbMIDI — keep as-is |

**Pitch Bend API change note:** In Teensyduino 1.41+, `sendPitchBend` accepts -8192 to +8191 (centered at 0). Earlier versions used 0–16383. The project should use -8192..+8191 and offset calculations accordingly. Confirm with `#include <usb_midi.h>` header or test a known value.

---

## PlatformIO Configuration — No Changes Required

The existing `platformio.ini` needs no modifications. All new MCU functionality uses:
- `usbMIDI` (built-in via `-DUSB_MIDI_SERIAL` flag — already set)
- `Bounce2` (already in `lib_deps`)
- `SoftPWM` (already in `lib_deps`)

No new library entries in `lib_deps` are needed.

---

## Sources

- [NicoG60/TouchMCU — mackie_control_protocol.md](https://github.com/NicoG60/TouchMCU/blob/main/doc/mackie_control_protocol.md) — MCU message reference, SysEx handshake sequence, challenge-response algorithm. **MEDIUM confidence** (community reverse-engineering of Logic Control documentation)
- [PJRC Teensyduino USB MIDI documentation](https://www.pjrc.com/teensy/td_midi.html) — sendPitchBend, sendNoteOn, sendSysEx, setHandleClock API. **HIGH confidence** (official)
- [PJRC cores/teensy3/usb_midi.h](https://github.com/PaulStoffregen/cores/blob/master/teensy3/usb_midi.h) — sendSysEx signature, USB_MIDI_SYSEX_MAX buffer constant (290 bytes). **HIGH confidence** (official source)
- [PJRC Teensy Forum — usbMIDI SysEx callback](https://forum.pjrc.com/threads/42589-usbMIDI-sysex-Callback-function) — setHandleSystemExclusive chunk handling, correct parameter types. **HIGH confidence** (official forum)
- [PJRC Teensy Forum — Sonar/Cakewalk Mackie SysEx](https://forum.pjrc.com/threads/50036-Sonar-Cakewalk-Mackie-SysEx) — Working MCU SysEx handshake implementation on Teensy. **MEDIUM confidence** (community-verified working code)
- [PJRC Teensy Forum — Receive Pitch Bend data (Mackie HUI)](https://forum.pjrc.com/index.php?threads/receive-pitch-bend-data-mackie-hui-protocol.50005/) — setHandlePitchChange API and fader receive pattern. **HIGH confidence** (official forum)
- [PJRC SoftPWM Library documentation](https://www.pjrc.com/teensy/td_libs_SoftPWM.html) — Uses PIT/IntervalTimer on Teensy 3.5, 30 Hz default, CPU-intensive. **HIGH confidence** (official)
- [Arduino Forum — Fader pickup/catch mode](https://forum.arduino.cc/t/fader-pickup-catch-mode/1206227) — Pickup mode state machine pattern. **MEDIUM confidence** (community)
- [Steinberg Remote Control Devices PDF](https://download.steinberg.net/downloads_software/documentation/Remote_Control_Devices.pdf) — Secondary reference for MCU protocol structure. **MEDIUM confidence**

---

*Stack research for: Phaedr MIDI Controller — MCU Protocol + LED Animation milestone*
*Researched: 2026-02-21*
