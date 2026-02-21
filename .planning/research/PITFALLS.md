# Pitfalls Research

**Domain:** Teensy 3.5 USB MIDI controller — Mackie Control Universal (MCU) protocol firmware
**Researched:** 2026-02-21
**Confidence:** MEDIUM — MCU protocol details verified against multiple sources (TouchMCU docs, libMackieControl, PJRC official docs, tttapa Control-Surface library); some Logic Pro-specific behaviors are LOW confidence due to limited first-party documentation

---

## Critical Pitfalls

### Pitfall 1: Incomplete SysEx Handshake Breaks Logic Pro Recognition

**What goes wrong:**
Logic Pro sends a Host Connection Query to the device at startup and expects a properly formed challenge-response reply within ~300ms. If the firmware ignores the query or sends a malformed response, Logic Pro may not display the Mackie Control surface, or may show it but not send any feedback (fader position, LED states, VPot ring values). The surface appears connected in the MIDI setup but is silent.

**Why it happens:**
Developers see that Logic Pro has a "Mackie Control" device type in its control surface preferences and assume sending the right MIDI notes/CC is sufficient. The SysEx handshake is not obvious from surface-level protocol documentation. The handshake was also removed in some later protocol revisions, causing confusion about whether it's required.

**The exact handshake sequence (MEDIUM confidence):**
1. Logic sends Device Query: `F0 00 00 66 14 00 F7`
2. Firmware must respond with Host Connection Query: `F0 00 00 66 14 01 <7-byte serial> <4-byte challenge> F7`
   - Device ID byte is `0x14` for Mackie Control Universal
   - Serial: any 7 ASCII bytes (e.g., your device name)
   - Challenge: 4 random bytes (0x00–0x7F each, no high bit)
3. Logic replies with Host Connection Reply: `F0 00 00 66 14 02 <same serial> <4-byte response> F7`
   - Response is computed from challenge using specific XOR/shift algorithm
4. Firmware must send Host Connection Confirmation: `F0 00 00 66 14 03 <same serial> F7`

The challenge-response algorithm:
```cpp
r[0] = 0x7F & (c[0] + (c[1] ^ 0x0A) - c[3]);
r[1] = 0x7F & ((c[2] >> 4) ^ (c[0] + c[3]));
r[2] = 0x7F & (c[3] - (c[2] << 2) ^ (c[0] | c[1]));
r[3] = 0x7F & (c[1] - c[2] + (0xF0 ^ (c[3] << 4)));
```
The firmware verifies Logic's response against locally-computed expected values and only sends the Confirmation if they match.

**How to avoid:**
Implement a full `handleSysEx` callback using `usbMIDI.setHandleSystemExclusive()`. Parse incoming SysEx to detect the Device Query (`14 00`) and Host Connection Reply (`14 02`). Implement the full challenge-response verification. Send the Confirmation only after a valid reply.

```cpp
// In setup():
usbMIDI.setHandleSystemExclusive(handleSysEx);

void handleSysEx(const byte* data, uint16_t length, bool complete) {
    // Check manufacturer ID: 00 00 66, device ID: 14
    if (length >= 6 && data[1]==0x00 && data[2]==0x00 && data[3]==0x66 && data[4]==0x14) {
        if (data[5] == 0x00) { // Device Query
            sendHostConnectionQuery();
        } else if (data[5] == 0x02) { // Host Connection Reply
            verifyAndConfirm(data);
        }
    }
}
```

**Warning signs:**
- Logic Pro shows the Mackie Control listed but nothing happens when you press play/stop on the controller
- Fader positions never update when Logic tracks change values
- VPot rings show no LED response even with plugin parameters open
- Serial debug shows incoming SysEx with `14 00` pattern but firmware doesn't respond

**Phase to address:** MCU Protocol Foundation (first phase — nothing else works without this)

---

### Pitfall 2: Faders Sending CC Instead of Pitch Bend on the Wrong Channel

**What goes wrong:**
MCU protocol maps faders to 14-bit pitch bend, not CC messages. Each of the 8 channel faders uses its own MIDI channel (fader 1 = channel 1, fader 2 = channel 2, ... fader 8 = channel 8). The existing firmware sends all CC messages on channel 1. If faders continue sending CC on channel 1, Logic ignores them entirely for fader control — they may trigger random parameter changes or nothing at all.

**Why it happens:**
The existing `Potentiometer` class calls `usbMIDI.sendControlChange()`. Switching to pitch bend requires a different API call and encoding. MCU's per-channel-per-fader assignment is counterintuitive: in standard MIDI you think of channel as instrument, not as "which fader."

**The correct encoding:**
```cpp
// For fader N (1-based), send pitch bend on MIDI channel N
// 14-bit value: map ADC 0-1023 to pitch bend 0-16383
uint16_t pitchVal = map(adcValue, 0, 1023, 0, 16383);
uint8_t lsb = pitchVal & 0x7F;        // lower 7 bits
uint8_t msb = (pitchVal >> 7) & 0x7F; // upper 7 bits
usbMIDI.sendPitchBend(pitchVal - 8192, faderChannel); // Teensy API centers at 0
```

**How to avoid:**
Create a separate `Fader` class (or extend `Potentiometer`) that sends pitch bend on a configurable channel. Do not try to reuse the CC-based Potentiometer for MCU faders.

**Warning signs:**
- Faders move but DAW parameters don't respond, or wrong parameters change
- Logic fader strips show no movement when physical sliders move
- MIDI monitor shows CC messages where pitch bend messages are expected

**Phase to address:** MCU Protocol Foundation

---

### Pitfall 3: VPot LED Ring Byte Encoding Wrong

**What goes wrong:**
The host DAW sends CC 48–55 to control VPot LED rings (one CC per knob). The CC value byte encodes both mode and position in a packed format. Getting this wrong produces wrong LED patterns or no LEDs at all.

**Why it happens:**
The encoding is not obvious. It's a packed byte, not a raw 0–127 value. Documentation is sparse and not in one place.

**The correct format (MEDIUM confidence, multiple sources agree):**
```
CC Value byte layout:
  Bits 7:   unused (always 0)
  Bits 6:   Center LED on/off (0 = off, 1 = on)
  Bits 5-4: Mode (00=single dot, 01=boost/cut fill, 10=wrap, 11=spread)
  Bits 3-0: Position value (0–11 for 11-LED ring)

Mode meanings:
  0b00 (0x00): Single LED dot at position
  0b01 (0x10): Fill from left up to position (like a volume bar)
  0b10 (0x20): Fill from center outward (boost/cut style)
  0b11 (0x30): Spread — two symmetrical LEDs from center

Example — position 6, boost/cut mode, center LED on:
  byte = (1 << 6) | (0b10 << 4) | 6 = 0x66
```

**How to avoid:**
Implement a `VPotState` struct that holds mode and position separately, and encodes to the CC byte only at send time. This makes intent clear and prevents bit-manipulation bugs.

```cpp
struct VPotState {
    uint8_t mode;     // 0-3
    uint8_t value;    // 0-11
    bool centerLED;

    uint8_t encode() const {
        return ((centerLED ? 1 : 0) << 6) | (mode << 4) | (value & 0x0F);
    }
};
```

**Warning signs:**
- All VPot LEDs off even when parameters are being controlled
- LEDs light up but in wrong pattern (single when you expect fill, etc.)
- Center LED never lights even when parameter is at noon

**Phase to address:** MCU Protocol Foundation

---

### Pitfall 4: Pickup Mode Fails at Boundary Values (0 and 127)

**What goes wrong:**
When the DAW value is at 0 and the physical fader is also at 0 (or both at 127), pickup mode can lock up permanently. The pickup algorithm waits for the physical value to cross the DAW value from one direction. At the boundary, there is no "other side" to cross from — the fader is already at the extreme. If the algorithm only watches for crossings (value went from below to above, or above to below), a fader sitting at 0 with DAW also at 0 should immediately pick up, but a naive implementation that requires a crossing event will never fire because the fader has nowhere to cross from.

**Why it happens:**
Standard pickup/soft-takeover implementations track which "side" the physical control is on relative to the DAW value and wait for a crossing. At the boundaries, the crossing can only come from one direction. If the fader is already at the boundary value matching the DAW value, the crossing condition may never evaluate as true because no change event occurs (the ADC reads the same value repeatedly and `hasChanged()` returns false).

**The edge cases:**
1. Physical at 0, DAW at 0 → should immediately pick up, but crossing never fires
2. Physical at 127, DAW at 127 → same problem
3. Physical at 127, DAW at 64 → fader must move DOWN to cross; if user pushes UP (already at max), fader is stuck — it can never cross from above
4. Physical at 0, DAW at 64 → fader must move UP; if already at min, pushing down does nothing

**How to avoid:**
Use a tolerance-based pickup rather than a pure crossing detection:

```cpp
bool pickupMode = true;
int dacValue = 64; // last known DAW value
bool pickedUp = false;

void Fader::read() {
    int raw = analogRead(_pin);
    int mapped = map(raw, 0, 1023, 0, 127);

    if (!pickedUp) {
        // Pick up if physical value is within tolerance of DAW value
        // OR if physical is at the same extreme as DAW value
        int tolerance = 3; // ADC noise margin
        bool atSameExtreme = (mapped <= tolerance && dacValue <= tolerance) ||
                             (mapped >= 127 - tolerance && dacValue >= 127 - tolerance);
        bool crossing = abs(mapped - dacValue) <= tolerance;

        if (atSameExtreme || crossing) {
            pickedUp = true;
        }
    }

    if (pickedUp && hasChanged(raw)) {
        sendFaderValue(mapped);
    }
}

// Reset pickedUp when DAW updates the value (bank switch, preset load)
void Fader::setDawValue(int value) {
    if (dacValue != value) {
        dacValue = value;
        pickedUp = false; // re-enter pickup mode
    }
}
```

**Warning signs:**
- After bank switch, fader that was at position 0 for both physical and DAW still doesn't respond to movement
- Fader at max position is permanently stuck and never picks up even when DAW value is 127
- LED blink indicating "out of sync" never clears despite fader being physically at the DAW value

**Phase to address:** Pickup Mode implementation phase

---

### Pitfall 5: Beat Chaser Drifts Because handleContinue Is Not Registered

**What goes wrong:**
Logic Pro sends `Start` (0xFA) when playback begins from position 1. But when resuming from any non-zero position — including loop cycle restarts and click-to-position-then-play — Logic sends `Continue` (0xFB) instead of `Start`. A firmware that only resets its clock counter on `Start` will have the beat chaser wildly out of phase when the user resumes mid-song, because the clock pulse counter starts from 0 even though the song is at beat 16 (or wherever).

**Why it happens:**
The existing codebase only has stubs for `handleStart`, `handleClock`, and `handleStop` — there is no `handleContinue` registered at all. The MIDI spec treats Start and Continue as distinct events with different implied beat positions.

**The correct behavior:**
- `Start` (0xFA): Reset clock counter to 0, start animation from beat 1
- `Continue` (0xFB): Do NOT reset counter, resume from current position
- `Stop` (0xFC): Pause animation, remember counter position
- `Clock` (0xF8): Increment counter, advance animation at 24 PPQN

```cpp
// In setup():
usbMIDI.setHandleContinue(handleContinue);

// Global state
volatile uint8_t clockCount = 0;
volatile bool playing = false;

void handleStart() {
    clockCount = 0;  // reset to beat 1
    playing = true;
}

void handleContinue() {
    // Do NOT reset clockCount — resume from wherever we are
    playing = true;
}

void handleStop() {
    playing = false;
    // Do NOT reset clockCount — preserve position for Continue
}

void handleClock() {
    if (!playing) return;
    clockCount++;
    if (clockCount >= 24) clockCount = 0;
    // Update beat chaser at clockCount == 0 (each new beat)
}
```

**Warning signs:**
- Beat chaser is in correct phase when starting from bar 1, but wrong when clicking mid-song
- Beat chaser jumps back to LED 0 on cycle loop restart (Logic sends Continue at loop point)
- Chaser animation visually stutters when Logic sends clock bursts during locate operations

**Phase to address:** Beat Chaser / Transport Sync phase

---

## Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Serial.println in every MIDI callback | Easy debug during dev | Adds 50–200µs per callback; causes MIDI jitter at 24 PPQN clock rate (PPQN fires 400 times/second at 100 BPM) | Development only — must be removed before shipping |
| Copy-by-value Button in InputManager (existing bug) | Compiles cleanly | LED feedback silently does nothing — setLedState writes to a temporary | Never acceptable — fix before adding MCU features |
| Hardcoded CC mapping alongside new MCU mapping | Quick to add MCU without full refactor | Two competing message protocols cause routing confusion; DAW gets duplicate messages | Never acceptable — migrate fully to MCU mapping |
| Reuse Potentiometer class for MCU faders | Fewer files | Class sends CC, not pitch bend; sends on wrong channel; no pickup state | Never — create a separate Fader class |
| `delay()` inside startup animation code | Easy to write sequenced animations | Blocks MIDI read loop; Logic Pro sends clock pulses during animation; missed pulses = lost beats | Never — use `millis()` state machines instead |
| Global volatile variables for clock counter | Simpler ISR-main sync | Unsafe multi-byte reads if counter exceeds `uint8_t` — compiler may read bytes in two operations; use atomic access or keep counter small | Acceptable only if counter stays within single-byte range (0–23 for PPQN is fine) |

---

## Integration Gotchas

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| Logic Pro MCU handshake | Not implementing SysEx reply at all; Logic shows surface but sends no feedback | Register `setHandleSystemExclusive`, respond to Device Query and verify challenge-response |
| Logic Pro transport | Only registering `handleStart` but not `handleContinue` | Register both; only reset beat counter on Start, not on Continue |
| Logic Pro fader feedback | Expecting CC feedback for fader position; Logic sends pitch bend on per-channel basis | Register `setHandlePitchChange` for channels 1–8; update fader DAW values from pitch bend messages |
| Logic Pro LED feedback | Expecting CC for LED state; MCU uses Note On/Off for button LEDs | Register `setHandleNoteOn` and `setHandleNoteOff`; look up note number in LED registry |
| usbMIDI rapid SysEx sending | Sending many SysEx messages in a burst overwhelms macOS | Add 1–2ms delay between sequential SysEx messages or pace with `elapsedMillis` |
| SysEx receive buffer size | Using simple `setHandleSystemExclusive` (2-arg) for large messages | Use chunk variant `setHandleSystemExclusiveChunk` (3-arg with `last` bool) to handle fragmentation; the simple callback has internal buffer size limits |

---

## Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| `Serial.println()` in `handleClock()` | MIDI clock jitter; beat chaser visually stutters; USB overload | Remove all Serial output from clock/start/stop callbacks; use a flag and print only in `loop()` | At 120 BPM, clock fires 48 times/second; even a 1ms Serial call = 4.8% loop time consumed |
| SoftPWM IntervalTimer on Teensy 3.5 | Intermittent crash after many timer interruptions (documented bug) | Keep total active IntervalTimers at 3 or fewer (SoftPWM uses 1, leaving 3 available for custom use) | Hits after thousands of activations in continuous use |
| Polling all 16 knobs/sliders on every clock callback | ADC reads block for ~10µs each; 16 reads = 160µs; at 24 PPQN, clock fires every ~5ms at 100 BPM | Never call `readAll()` from clock ISR; keep clock callback to counter increment only; do ADC polling in main loop | At 120 BPM, clock arrives every 4.2ms — 160µs is 4% overhead per tick |
| Startup animation using `for` loop with `delay()` | Entire MIDI stack frozen during animation; any messages from Logic during boot are dropped | Use a state machine in `loop()` with `millis()` timestamps; advance animation state each iteration | Immediately — Logic may send clock or request handshake during animation |
| Non-volatile `clockCount` shared between ISR and main loop | Race condition: main loop reads partial value while ISR updates it; animation jumps | Declare `volatile uint8_t clockCount`; if counter must exceed 255, use `noInterrupts()` guard around reads in main loop | Random, hard to reproduce — typically shows as animation position briefly jumping |
| Sending MIDI from within a MIDI callback | Teensy's usbMIDI is not re-entrant; calling `sendControlChange()` inside a clock handler can corrupt internal USB buffers | Set a flag inside the callback; in `loop()`, check flag and send MIDI there | Intermittent USB corruption; harder to trace |

---

## UX Pitfalls

| Pitfall | User Impact | Better Approach |
|---------|-------------|-----------------|
| LED blink for pickup mode uses SoftPWM fade timing (125ms) | Blink appears to lag behind actual fader position; hard to see at 2Hz | Set blink period to 500ms using `millis()` state machine; use `SoftPWMSet(pin, 255)` / `SoftPWMSet(pin, 0)` toggled manually rather than relying on SoftPWM fade |
| Startup animation runs before MCU handshake completes | Animation finishes, LEDs settle, then Logic sends initial state and LEDs change again — looks like a glitch | Run startup animation before registering MCU handlers, OR after receiving first fader position update from Logic |
| Beat chaser overwrites transport state LEDs | Record button LED goes dark mid-record because beat chaser thinks it's "its turn" on that grid cell | Never allow beat chaser to write to cells that are owned by transport state (Record, Play, Stop LEDs are protected) |
| Shift held dims ALL LEDs including active transport buttons | Users can't see which transport buttons are active while shift is held | Dim only non-transport LEDs; transport LEDs (Play, Record, etc.) should remain at full brightness during shift |
| VPot LED ring doesn't update until knob is touched | User changes a plugin parameter by mouse, knob LED ring stays at old position | Subscribe to incoming VPot CC (48–55) from Logic and update ring on DAW-initiated changes, not just hardware knob turns |

---

## "Looks Done But Isn't" Checklist

- [ ] **MCU handshake:** SysEx reply sends confirmation AND waits for Logic's response — verify by checking MIDI monitor that 3-step sequence completes before any other MCU traffic
- [ ] **Fader pickup mode:** Test with fader physically at 0 and DAW value at 0; moving fader up should immediately respond — if it doesn't respond for first 10% of travel, boundary bug is present
- [ ] **Beat chaser with Continue:** Press play from bar 1 (Start message), then stop, then click to bar 5 and press play again (Continue message) — chaser should NOT reset to beat 1
- [ ] **Transport LED feedback:** Press Record in Logic; Record LED on hardware should light. Stop playback; Play LED should extinguish. These require Note On/Off handlers, not CC handlers.
- [ ] **Startup animation blocking:** During animation, send a MIDI clock message via a secondary device — verify it's received and queued, not dropped
- [ ] **Pickup LED blink:** Bank switch, verify channel button blinks. Move fader to DAW position, verify blink stops exactly at crossing — not before, not after
- [ ] **VPot ring modes:** Open an EQ plugin in Logic. Verify a Q knob shows spread mode, gain shows boost/cut fill from center — these require correct mode bits per parameter type

---

## Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Wrong SysEx handshake | MEDIUM | Add SysEx handler, implement challenge-response; test by watching Logic's MIDI monitor for all 3 handshake messages |
| Fader sending CC instead of pitch bend | MEDIUM | Create Fader class replacing Potentiometer for sliders; update InputManager initialization |
| VPot encoding wrong | LOW | Fix encode() function; regression test against known-good CC values from real MCU (capture with MIDI monitor) |
| Pickup boundary lockup | LOW | Add tolerance check alongside crossing check; unit-testable without hardware |
| Beat chaser drift (missing Continue) | LOW | Add `setHandleContinue()` registration; test by looping a 4-bar region and watching chaser |
| Animation blocking MIDI | HIGH | Requires rewriting all animations as state machines; the larger the animation the harder the rewrite — do this right the first time |

---

## Pitfall-to-Phase Mapping

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| Incomplete SysEx handshake | Phase 1: MCU Protocol Foundation | MIDI monitor shows all 3 handshake messages; Logic Control Surface pane shows surface as active |
| Fader CC instead of pitch bend | Phase 1: MCU Protocol Foundation | Logic fader strips respond to physical slider movement |
| VPot LED ring encoding | Phase 1: MCU Protocol Foundation | VPot LEDs respond with correct pattern when Logic sends CC 48–55 |
| Pickup boundary lockup | Phase 2: Pickup Mode | Manual test: fader at 0 with DAW at 0 immediately responds; blink clears at crossing |
| Missing handleContinue | Phase 3: Beat Chaser / Transport | Mid-song resume test: chaser stays in phase |
| Animation blocking MIDI | Phase 4: Startup & Action Animations | Stress test: send MIDI clock during animation; verify no clock pulses dropped |
| Serial in callbacks | Phase 1 cleanup (before any timing work) | Remove all Serial from callback bodies; only log in main loop via flag |
| Copy-by-value Button bug | Pre-MCU cleanup (before Phase 1) | LED feedback from DAW actually updates physical LEDs |

---

## Sources

- [TouchMCU Mackie Control Protocol Documentation](https://github.com/NicoG60/TouchMCU/blob/main/doc/mackie_control_protocol.md) — SysEx handshake sequence, challenge/response algorithm, VPot encoding (MEDIUM confidence)
- [libMackieControl Protocol Documentation](https://github.com/Do-sth-sharp/libMackieControl/blob/main/doc/MackieControl.md) — Fader channel assignment, VPot CC range, transport note numbers, LED note numbers (MEDIUM confidence)
- [PJRC Teensy USB MIDI Documentation](https://www.pjrc.com/teensy/td_midi.html) — SysEx chunk callback, rapid message limitations, real-time callback registration (HIGH confidence — official PJRC docs)
- [PJRC SoftPWM Library Documentation](https://www.pjrc.com/teensy/td_libs_SoftPWM.html) — IntervalTimer usage, CPU overhead warning, latency-sensitive library conflict list (HIGH confidence — official PJRC docs)
- [PJRC IntervalTimer Documentation](https://www.pjrc.com/teensy/td_timing_IntervalTimer.html) — 4-timer limit on Teensy 3.x, ISR context restrictions, volatile requirement, end() race condition (HIGH confidence — official PJRC docs)
- [tttapa Control-Surface VPotRing/VPotState Reference](https://tttapa.github.io/Control-Surface-doc/Doxygen/d5/ddb/classMCU_1_1VPotRing.html) — VPot mode encoding verification (MEDIUM confidence)
- [Teensy Forum: IntervalTimer crash on 3.5/3.6](https://forum.pjrc.com/index.php?threads/teensy-3-6-3-5-intervaltimer-sketch-crashes-after-thousands-of-activations.56997/) — Known IntervalTimer stability issue on Teensy 3.5 (MEDIUM confidence — community report, not official bug tracker)
- [Apple Logic Pro MIDI Sync Documentation](https://support.apple.com/guide/logicpro/midi-synchronization-settings-lgcp72142361/10.7/mac/11.0) — Start vs Continue vs Song Position Pointer behavior (HIGH confidence — official Apple docs)
- Logic Pro Control Surfaces Support Guide (PDF) — MCU overview, transport controls (HIGH confidence — official Apple docs)
- [Zynthian Forum: MCU Device IDs](https://discourse.zynthian.org/t/mackie-control-as-supported-midi-controller/11717/15) — Device ID 0x14 for MCU Universal confirmed (MEDIUM confidence)

---

*Pitfalls research for: Teensy 3.5 MCU protocol firmware (Mackie Control Universal + Logic Pro)*
*Researched: 2026-02-21*
