# Phaedr MIDI Controller

![Phaedr MIDI Controller](./assets/cover.png)

See the [hackaday](https://hackaday.io/project/204994-phaedr-midi-controller) project

USB MIDI controller firmware for Teensy 3.5. A custom control surface with 16 grid buttons, 8 track buttons, 8 rotary knobs, and 8 sliders — designed for bidirectional communication with a DAW.

This project started in 2021 as a pandemic side project — custom PCB, custom enclosure, and firmware from scratch. The goal was less about building the most practical MIDI controller and more about a fun design challenge: use a Teensy 3.5 and max out every available pin. I got distracted along the way and never finished it, but now with the help of Claude Code I'm eager to wrap up the loose ends. The Teensy 3.5 is largely deprecated at this point, though most of the firmware should be portable to a Teensy 4.1 with pin remapping.

## Features

- USB MIDI over serial — no external MIDI interface needed
- Bidirectional CC feedback: button LEDs reflect DAW state
- PWM LED fading via SoftPWM
- Debounced button input via Bounce2
- Noise-filtered analog inputs for knobs and sliders

## Hardware

- **MCU:** Teensy 3.5
- **16 grid buttons** with individual LEDs (CC 102–117)
- **8 track buttons** with individual LEDs (CC 20–27)
- **8 rotary knobs** (CC 14–15, 28–31, 118–119)
- **8 sliders** (CC 3, 9, 85–90)

All MIDI messages use channel 1.

## Build

Requires [PlatformIO](https://platformio.org/).

```sh
# Build
pio run

# Upload to Teensy
pio run --target upload

# Serial monitor (for debug output)
pio device monitor
```

## Dependencies

Managed automatically by PlatformIO:

- [Bounce2](https://github.com/thomasfredericks/Bounce2) — button debouncing
- [SoftPWM](https://github.com/bhagman/SoftPWM) — PWM LED control
