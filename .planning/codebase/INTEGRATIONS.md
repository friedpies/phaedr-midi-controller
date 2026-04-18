# External Integrations

**Analysis Date:** 2026-03-02

## APIs & External Services

**MIDI Protocol:**
- USB MIDI over native Teensy USB endpoint
  - SDK/Client: Teensyduino `usbMIDI` object (built-in to Arduino framework)
  - Channel: MIDI channel 1 (all messages)
  - Protocol: Standard USB MIDI class-compliant device
  - No authentication required

**Mackie Control Universal (MCU) Protocol:**
- Handshake via SysEx (System Exclusive)
  - Header: F0 00 00 66 14 (Mackie MCU device ID)
  - Device Identity: "PHAEDR\0" (7-byte serial)
  - Handshake: Challenge-response authentication (4-byte static challenge: 0x7A 0x6B 0x5C 0x4D)
  - Implementation: `src/mcuProtocol.h/cpp`
  - Status: Required for handshake before input/output; retry every 5 seconds if DAW doesn't respond

## Data Storage

**Databases:**
- Not applicable

**File Storage:**
- Local firmware only; no persistent storage beyond microcontroller flash

**Caching:**
- Not applicable (real-time control surface, no network I/O)

## Authentication & Identity

**Auth Provider:**
- Custom MCU Protocol challenge-response
  - Implementation: `src/mcuProtocol.cpp` - validateChallengeResponse()
  - Device identity hardcoded: "PHAEDR\0"
  - Challenge bytes: static (0x7A, 0x6B, 0x5C, 0x4D)
  - No user login; DAW (Logic Pro, Ableton, etc.) responds to handshake

## Monitoring & Observability

**Error Tracking:**
- None configured

**Logs:**
- Serial debug output (optional, enabled via `-DUSB_MIDI_SERIAL` build flag)
  - Monitor with: `pio device monitor`
  - No persistent logging

## CI/CD & Deployment

**Hosting:**
- Embedded firmware (Teensy 3.5 microcontroller) — no cloud deployment
- Local build and upload via USB cable

**CI Pipeline:**
- None configured

**Upload:**
- `pio run --target upload` triggers Teensy bootloader automatically

## Environment Configuration

**Required env vars:**
- None

**Secrets location:**
- No secrets; MCU challenge bytes are static and hardcoded in firmware

## Webhooks & Callbacks

**Incoming:**
- MIDI Note On/Off (from DAW/Logic Pro)
  - Handler: `handleNoteOn()`, `handleNoteOff()` in `src/main.cpp`
  - Routes to: `InputManager.handleNoteMessage()` → `NoteRegistry` → LED state updates
  - Use case: DAW sends note on/off to update button LEDs based on transport/track state

- MIDI Pitch Bend (from DAW/Logic Pro, channels 1-8)
  - Handler: `handlePitchBend()` in `src/main.cpp`
  - Routes to: `InputManager.setFaderDawValue()` → Fader pickup mode
  - Use case: DAW sends fader position feedback (14-bit value) for pickup detection and bank switching
  - Range: Channels 1-8 map to fader 1-8; value +8192 offset to -8192..+8191 signed range

- MIDI SysEx (System Exclusive)
  - Handler: `handleSysEx()` in `src/main.cpp`
  - Routes to: `MCUProtocol.handleSysEx()` → handshake authentication
  - Use case: DAW responds to MCU protocol handshake query

**Outgoing:**
- MIDI CC (Control Change)
  - Sent on button/knob/slider change
  - Grid buttons: CC 102-117 (velocity 127 = pressed, 0 = released)
  - Track buttons: MIDI Note On/Off (notes 0-7 = Arm, 8-15 = Solo, 16-23 = Mute, 46-47 = Bank navigation)
  - Knobs: CC 16-23 (relative VPot encoder format: 0x01 = CCW -1 step, 0x41 = CW +1 step)
  - Sliders (Faders): MIDI Pitch Bend on channels 1-8 (14-bit value 0-16383)

- MIDI SysEx (System Exclusive)
  - Sent during handshake: unsolicited query to DAW to initiate authentication
  - Message format: F0 00 00 66 14 [identity] [challenge] F7
  - Retry: Every 5 seconds if no response, until handshake completes

## MIDI CC Mapping Reference

**Input → DAW (Controller sends):**

| Type | Count | MIDI Channel | Values/Format | Notes |
|------|-------|--------------|---------------|-------|
| Grid buttons | 16 | 1 | CC 102-117 (vel 127/0) | Button press/release |
| Track buttons (Arm) | 8 | 1 | Note 0-7 (vel 127/0) | MCU protocol: note on = armed |
| Track buttons (Solo) | 8 | 1 | Note 8-15 (vel 127/0) | MCU protocol |
| Track buttons (Mute) | 8 | 1 | Note 16-23 (vel 127/0) | MCU protocol |
| Bank Left | 1 | 1 | Note 46 (vel 127) | Bank shift << |
| Bank Right | 1 | 1 | Note 47 (vel 127) | Bank shift >> |
| Knobs (VPot) | 8 | 1 | CC 16-23 (rel: 0x01=CCW, 0x41=CW) | Relative encoder format |
| Sliders (Faders) | 8 | 1-8 | Pitch Bend (14-bit 0-16383) | Channel = fader index + 1 |

**Output ← DAW (DAW sends to update LEDs):**

| Type | MIDI Type | Channel | Values | Notes |
|------|-----------|---------|--------|-------|
| Grid button LEDs | Note On | 1 | Notes (various) | Velocity: 127=on, 0=off, 1=blink (Phase 2) |
| Track button LEDs | Note On | 1 | Notes (various) | Velocity: 127=on, 0=off |
| Fader feedback | Pitch Bend | 1-8 | 14-bit 0-16383 | Channel = fader index + 1; triggers pickup mode |

## Hardware Interfaces

**GPIO (General Purpose I/O):**
- 24 digital input pins: button switches (16 grid + 8 track buttons)
  - Pin assignments: `src/pinDefines.h` (K1_SW-K16_SW, P1_SW-P8_SW)
  - Debounce: 20ms (via Bounce2 library)

- 24 digital output pins: LED drivers (16 grid + 8 track LEDs)
  - Pin assignments: `src/pinDefines.h` (LED_K1-LED_K16, LED_P1-LED_P8)
  - Control: SoftPWM with brightness 0-180 (180/255 = 70.6% duty cycle for power budget)
  - Fade support: SoftPWM allows gradual brightness transitions

**ADC (Analog-to-Digital Converter):**
- 8 knob analog inputs (10-bit resolution, 0-1023 ADC units)
  - Pin assignments: `src/pinDefines.h` (KNOB_1-KNOB_8: pins 15-19, A10-A12)
  - Noise threshold: 3 ADC units (±1.5 units tolerance)
  - Relative encoder tuning: ADC_PER_STEP = 8 (1023 / 8 ≈ 128 steps per full sweep)

- 8 slider analog inputs (10-bit resolution, 0-1023 ADC units)
  - Pin assignments: `src/pinDefines.h` (SLIDE_1-SLIDE_8: pins A13-A15, A20-A22, A0, A6)
  - Noise threshold: 48 ADC units (proportional to 3-unit knob threshold, mapped to 14-bit MIDI range 0-16383)
  - Pickup mode: Detects bank switches (>1024 unit gap between physical and DAW position)

---

*Integration audit: 2026-03-02*
