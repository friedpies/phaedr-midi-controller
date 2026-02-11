# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Teensy 3.5 USB MIDI controller firmware. A hardware control surface with 16 grid buttons (with LEDs), 8 track buttons (with LEDs), 8 rotary knobs, and 8 sliders — all communicating over USB MIDI. Built for bidirectional communication with a DAW (e.g., Ableton): the controller sends CC messages on input, and receives CC feedback to update LED states.

## Build Commands

This is a PlatformIO project. All commands use the `pio` CLI (install via `pip install platformio` or use the PlatformIO IDE extension).

- **Build:** `pio run`
- **Upload to Teensy:** `pio run --target upload`
- **Clean:** `pio run --target clean`
- **Serial monitor:** `pio device monitor`

Target environment is `teensy35` with Arduino framework. The build flag `-DUSB_MIDI_SERIAL` enables USB MIDI + Serial simultaneously.

## Dependencies

Defined in `platformio.ini`:
- **Bounce2** (^2.70) — button debouncing
- **SoftPWM** (^1.0.1) — PWM-based LED fading

## Architecture

All source is in `src/`. The firmware follows Arduino's `setup()`/`loop()` pattern.

**main.cpp** — Entry point. Initializes `InputManager`, registers USB MIDI handlers (CC, transport start/clock/stop). The main loop calls `inputManager.readAll()` and `usbMIDI.read()`.

**InputManager** (`inputManager.h/cpp`) — Central orchestrator. Owns all input arrays:
- 16 `Button` grid buttons (CC 102–117)
- 8 `Button` track buttons (CC 20–27)
- 8 `Potentiometer` knobs (CC 14–15, 28–31, 118–119)
- 8 `Potentiometer` sliders (CC 3, 9, 85–90)

Each input is constructed with its hardware pin, CC number, and config. `readAll()` polls every input each loop iteration. `handleControlChangeMessage()` routes incoming CC from the DAW to the `ButtonRegistry` for LED feedback.

**Button** (`button.h/cpp`) — Wraps a physical button + LED pair. Uses Bounce2 for debouncing (20ms). On press, toggles state and sends CC 127 (on) or 0 (off). LED brightness is controlled via SoftPWM with fade support.

**Potentiometer** (`potentiometer.h/cpp`) — Reads an analog pin, maps 10-bit ADC to 7-bit MIDI (0–127), filters noise with a 3-value threshold. Supports inversion (used by knobs).

**ButtonRegistry** (`buttonRegistry.h/cpp`) — Maps CC numbers → Button pointers via `std::map`. Enables DAW-to-hardware LED feedback: when a CC message arrives, the registry looks up the corresponding button and updates its LED.

**pinDefines.h** — Hardware pin assignments for all switches, LEDs, knobs, and sliders. Pin names follow the convention `K1_SW`/`LED_K1` (grid), `P1_SW`/`LED_P1` (track), `KNOB_1`, `SLIDE_1`.

## MIDI CC Mapping

| Input Type | Count | CC Numbers |
|---|---|---|
| Grid buttons | 16 | 102–117 |
| Track buttons | 8 | 20–27 |
| Knobs | 8 | 14–15, 28–31, 118–119 |
| Sliders | 8 | 3, 9, 85–90 |

All messages are sent/received on MIDI channel 1.
