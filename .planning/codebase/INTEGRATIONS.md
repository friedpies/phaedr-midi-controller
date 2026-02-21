# External Integrations

**Analysis Date:** 2026-02-20

## APIs & External Services

**MIDI Communication:**
- USB MIDI Protocol - Bidirectional communication with host DAW
  - SDK/Client: Native Teensy MIDIUSB library (built-in to Teensy core)
  - Connection method: USB 2.0 connection to host computer
  - Protocol: MIDI (Musical Instrument Digital Interface) over USB
  - Purpose: Send CC (Control Change) messages from hardware to DAW; receive CC messages from DAW to update LED states

**DAW Integration (Supported but not hardcoded):**
- Ableton Live - Primary reference DAW for controller mapping
  - Integration via: Standard MIDI CC messaging on channel 1
  - No API integration - uses standardized MIDI protocol
  - Supports any DAW that accepts USB MIDI input and outputs MIDI CC messages

## Data Storage

**Databases:**
- None - Firmware does not use any database

**File Storage:**
- None - Firmware does not use file storage

**Caching:**
- Runtime state only - Button states and potentiometer values held in memory
  - Grid button states: 16 x 1 byte (toggle state)
  - Track button states: 8 x 1 byte (toggle state)
  - Potentiometer values: 16 x 1 byte (last MIDI value, 0-127)
  - No persistent storage

## Authentication & Identity

**Auth Provider:**
- None - No authentication required. Firmware communicates directly over USB to host.

**Security Model:**
- USB trust model - Host computer and controller establish trust via USB connection
- No user accounts, tokens, or identity verification
- MIDI channel 1 is hardcoded; all communication on this channel

## Monitoring & Observability

**Error Tracking:**
- None - No external error tracking service

**Logs:**
- Serial output to USB Serial port when `-DUSB_MIDI_SERIAL` build flag is enabled
  - Location: Serial monitor via `pio device monitor`
  - Debug messages: MIDI events logged via `Serial.println()` statements
  - Examples in `src/main.cpp`:
    - `Serial.println("CONTROL CHANGE")` when CC message received
    - `Serial.println("HANDLE START")` on MIDI Start message
    - `Serial.println("HANDLE CLOCK")` on MIDI Clock message

**No Cloud Monitoring:**
- No telemetry, analytics, or remote monitoring

## CI/CD & Deployment

**Hosting:**
- Embedded firmware on Teensy 3.5 microcontroller
- No cloud hosting required
- No remote deployment capability

**CI Pipeline:**
- None - No CI/CD infrastructure detected
- Manual build and upload via PlatformIO CLI
- No automated testing infrastructure

**Firmware Updates:**
- Manual: USB upload via `pio run --target upload`
- Each update requires physical USB connection and re-programming of microcontroller

## Environment Configuration

**Required Environment Variables:**
- None - Firmware does not use environment variables
- All configuration is compiled into firmware at build time

**Hardware Pin Configuration:**
- Defined in compile-time in `src/pinDefines.h`
- Cannot be changed without recompilation
- Pin assignments cannot be externalized

**Secrets Location:**
- None - No secrets, API keys, or credentials used
- No `.env` file required or supported

## Webhooks & Callbacks

**Incoming Webhooks:**
- None - Firmware does not receive HTTP webhooks
- Receives MIDI CC messages over USB (not HTTP)

**Outgoing Webhooks:**
- None - Firmware does not send HTTP requests

**MIDI Callbacks (Hardware Events):**
- USB MIDI handlers registered in `src/main.cpp`:
  - `handleControlChangeMessage()` - Incoming CC messages from DAW (LED feedback)
  - `handleStart()` - MIDI Start message (transport control)
  - `handleClock()` - MIDI Clock message (timing sync)
  - `handleStop()` - MIDI Stop message (transport control)

## MIDI CC Message Map

**Grid Buttons (16):**
- CC 102-117 - Send CC 127 (on) or 0 (off) to DAW

**Track Buttons (8):**
- CC 20-27 - Send CC 127 (on) or 0 (off) to DAW

**Rotary Knobs (8):**
- CC 14-15, 28-31, 118-119 - Send values 0-127 to DAW

**Sliders (8):**
- CC 3, 9, 85-90 - Send values 0-127 to DAW

**All messages:** Channel 1, standardized MIDI CC format

---

*Integration audit: 2026-02-20*
