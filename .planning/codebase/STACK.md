# Technology Stack

**Analysis Date:** 2026-02-20

## Languages

**Primary:**
- C++ (C++11 or later) - Firmware implementation for Teensy microcontroller
- Arduino - Hardware abstraction and board support

**Secondary:**
- Python - PlatformIO build system configuration and scripting

## Runtime

**Environment:**
- Teensy 3.5 microcontroller (ARM Cortex-M4, 72 MHz, 256KB RAM)
- Arduino framework for Teensy

**Package Manager:**
- PlatformIO - Embedded systems build and dependency management
- Version detection: Defined via `platformio` CLI (install via `pip install platformio`)

## Frameworks

**Core:**
- Arduino Framework - Hardware abstraction layer for microcontroller I/O, timing, and serial communication
- Teensy Board Support - Specialized Arduino implementation for Teensy 3.5 with native USB MIDI support

**Hardware Abstraction:**
- `<Arduino.h>` - GPIO, serial, ADC, timing functions

## Key Dependencies

**Critical:**
- Bounce2 (v2.70) - Button debouncing library for physical switch input stability
  - Used in: `src/button.h`, `src/button.cpp`
  - Purpose: Eliminates contact bounce noise on mechanical buttons with 20ms debounce window

- SoftPWM (v1.0.1) - Software PWM (Pulse Width Modulation) for LED brightness control
  - Used in: `src/button.h`, `src/button.cpp`
  - Purpose: Provides LED fade/brightness effects without requiring dedicated hardware PWM pins

**MIDI Communication:**
- MIDIUSB (built-in to Teensy core) - Native USB MIDI protocol implementation
  - Used in: `src/main.cpp`, `src/button.h`, `src/buttonRegistry.cpp`
  - Purpose: Bidirectional USB MIDI communication with DAW (Ableton, etc.)

- MIDI Library (implicitly included via `<MIDI.h>` and `<MIDI.hpp>`)
  - Used in: `src/main.cpp`
  - Purpose: MIDI protocol parsing and message handling

## Configuration

**Environment:**
- Build flag: `-DUSB_MIDI_SERIAL` enables simultaneous USB MIDI and Serial communication
- Allows debugging via Serial monitor while MIDI I/O operates on USB

**Build Configuration:**
- `platformio.ini` - PlatformIO project configuration
  - Platform: `teensy`
  - Board: `teensy35`
  - Framework: `arduino`
  - Library dependencies defined in `lib_deps` section

**Hardware Pins:**
- All pin assignments defined in `src/pinDefines.h`
- 16 grid button switches with corresponding LEDs
- 8 track button switches with corresponding LEDs
- 8 knobs (rotary potentiometers)
- 8 sliders (linear potentiometers)

## Platform Requirements

**Development:**
- PlatformIO CLI or PlatformIO IDE extension
- Python 3.x (for PlatformIO)
- Teensy bootloader/upload capability
- USB cable for programming and power
- Teensyduino libraries (installed via PlatformIO)

**Build Commands:**
- Build: `pio run`
- Upload to Teensy: `pio run --target upload`
- Clean: `pio run --target clean`
- Serial monitor: `pio device monitor`

**Production/Deployment:**
- Target platform: Teensy 3.5 microcontroller
- USB MIDI interface to host computer (DAW)
- No external network connectivity required
- No cloud integration required
- Standalone firmware deployment via USB programming

---

*Stack analysis: 2026-02-20*
