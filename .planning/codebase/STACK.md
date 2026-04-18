# Technology Stack

**Analysis Date:** 2026-03-02

## Languages

**Primary:**
- C++ (Arduino dialect) - All firmware code (src/*.cpp and src/*.h)

**Secondary:**
- Arduino Sketch - Arduino-style setup()/loop() pattern in `src/main.cpp`

## Runtime

**Environment:**
- Teensyduino (Arduino framework for Teensy microcontrollers)
- Teensy 3.5 ARM Cortex-M4 @ 120 MHz
- Flash: 512 KB, RAM: 192 KB

**Package Manager:**
- PlatformIO (pio CLI)
- Lockfile: `platformio.lock` (managed automatically by PlatformIO)

## Frameworks

**Core:**
- Arduino Framework (via PlatformIO/Teensyduino) - Microcontroller programming model with setup()/loop()
- Teensyduino USB Stack - Native USB MIDI and Serial support via `usbMIDI` object

**Input/Output:**
- Bounce2 2.70 - Button debouncing with configurable debounce time
- SoftPWM 1.0.1 (vendored locally) - PWM-based LED brightness/fading control

**Build/Dev:**
- PlatformIO - Build system and uploader
- platformio.ini - Project configuration

## Key Dependencies

**Critical:**
- `Bounce2` (^2.70) - Debounces 24 button inputs (16 grid + 8 track). Required for stable button reads. Installation: `thomasfredericks/Bounce2@^2.70`
- `SoftPWM` (1.0.1) - Drives 24 LED outputs (22 channels: K1-K16 grid LEDs + P1-P8 track LEDs) with PWM fading. Vendored locally in `lib/SoftPWM/` with custom compile flag `SOFTPWM_MAXCHANNELS=22` (increased from upstream default of 20).

**Infrastructure:**
- Teensyduino USB MIDI Stack (built into Arduino framework) - USB MIDI endpoint + optional serial debug

## Configuration

**Environment:**
- No .env files
- No external configuration required
- Build flag: `-DUSB_MIDI_SERIAL` (platformio.ini line 20) enables simultaneous USB MIDI + serial debug output

**Build:**
- `platformio.ini` - Single environment: `[env:teensy35]`
  - platform: teensy
  - board: teensy35
  - framework: arduino
  - lib_deps: Bounce2, SoftPWM (vendored)
  - build_flags: `-DUSB_MIDI_SERIAL`

**Hardware Configuration:**
- GPIO pin mappings: `src/pinDefines.h` (24 button pins + 24 LED pins + 8 knob ADC pins + 8 slider ADC pins)
- LED brightness limit: `LED_MAX_BRIGHTNESS = 180` (70.6% PWM duty to stay within 500mA USB power budget)

## Platform Requirements

**Development:**
- PlatformIO installed: `pip install platformio`
- USB cable to Teensy 3.5
- Optional: PlatformIO IDE extension for VS Code / alternative IDE support

**Production:**
- Teensy 3.5 microcontroller board
- Custom PCB with:
  - 16 grid buttons + 16 LEDs
  - 8 track buttons + 8 LEDs
  - 8 rotary potentiometer knobs (analog inputs)
  - 8 linear faders (analog inputs)
- USB host with USB MIDI driver support (DAW such as Ableton Live, Logic Pro, etc.)
- USB power: ~489 mA max (24 LEDs × 14.1 mA avg + 150 mA Teensy = ~489 mA)

---

*Stack analysis: 2026-03-02*
