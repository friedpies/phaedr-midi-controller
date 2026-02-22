---
phase: 01-mcu-protocol-foundation
plan: "02"
subsystem: mcu-button-input
tags: [mcu, button, led, note-bang, registry, softpwm, usb-power]
dependency_graph:
  requires: []
  provides: [MCUButton, NoteRegistry, LED_MAX_BRIGHTNESS]
  affects: [inputManager, main]
tech_stack:
  added: []
  patterns: [note-bang, led-feedback-decoupled, note-to-pointer-map]
key_files:
  created:
    - src/mcuButton.h
    - src/mcuButton.cpp
    - src/noteRegistry.h
    - src/noteRegistry.cpp
  modified:
    - src/pinDefines.h
decisions:
  - "MCUButton is a clean standalone class (not subclass of Button) — structural enforcement of no-LED-self-toggle invariant"
  - "LED_MAX_BRIGHTNESS 180 caps SoftPWM at 70.6% duty to keep 24-LED draw within USB 500mA budget"
  - "_hasLed = (ledPin >= 0) guards all SoftPWM calls — transport buttons without LEDs pass ledPin=-1"
  - "NoteRegistry uses std::map<uint8_t, MCUButton*> matching ButtonRegistry pattern but keyed on note number"
metrics:
  duration: "~3 minutes"
  completed: "2026-02-22"
  tasks_completed: 2
  files_created: 4
  files_modified: 1
---

# Phase 01 Plan 02: MCUButton + NoteRegistry Summary

MCUButton class with Note Bang on press (NoteOn+NoteOff, no LED self-toggle) and NoteRegistry note-to-pointer map for Logic Pro LED feedback routing.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Add LED_MAX_BRIGHTNESS and MCUButton class | ef8e27b | src/pinDefines.h, src/mcuButton.h, src/mcuButton.cpp |
| 2 | Create NoteRegistry | 1feb716 | src/noteRegistry.h, src/noteRegistry.cpp |

## What Was Built

### MCUButton (src/mcuButton.h + src/mcuButton.cpp)

A clean replacement for the `Button` class in MCU mode. Key invariants enforced structurally:

- `read()` sends a Note Bang on button press: `sendNoteOn(note, 127, 1)` immediately followed by `sendNoteOff(note, 0, 1)`. No LED state change occurs in `read()`.
- `setLedState(velocity)` is the only path to the hardware LED. `velocity=127` → `SoftPWMSet(pin, LED_MAX_BRIGHTNESS)`. `velocity=0` → `SoftPWMSet(pin, 0)`. `velocity=1` (blink) is deferred to Phase 2.
- `_hasLed = (ledPin >= 0)` guard: transport buttons constructed with `ledPin=-1` skip all SoftPWM calls.

### LED_MAX_BRIGHTNESS (src/pinDefines.h)

Added `#define LED_MAX_BRIGHTNESS 180` with USB power budget comment. 180/255 = 70.6% duty cycle keeps worst-case current draw at ~489mA (24 LEDs × 14.1mA avg + 150mA Teensy) within the USB 500mA limit.

### NoteRegistry (src/noteRegistry.h + src/noteRegistry.cpp)

Note-to-MCUButton* lookup map for LED feedback routing from Logic Pro:

- `registerButton(noteNum, MCUButton*)` stores pointer in `std::map<uint8_t, MCUButton*> noteToButton`
- `getButton(noteNum)` returns `MCUButton*` or `nullptr` on miss
- MCU note number reference table included as block comment in the header for Plan 05 wiring

## Decisions Made

1. MCUButton is a standalone class, not a subclass of Button. The class boundary enforces the "no LED self-toggle" invariant at compile time — there is no toggle state or toggle path to accidentally call.

2. `LED_MAX_BRIGHTNESS 180` placed in `pinDefines.h` (not `mcuButton.h`) so the constant is available globally across all LED-touching code in the project.

3. `_hasLed = (ledPin >= 0)` pattern chosen over a separate boolean constructor parameter — ledPin value conveys both the pin number and whether an LED exists, reducing constructor surface.

4. NoteRegistry is a separate class from ButtonRegistry (not an extension). The CC-based and note-based registries serve different protocol layers and will coexist during the transition.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Stale `.pio` build cache contained SERIAL symbol conflict**

- **Found during:** Task 2 build verification
- **Issue:** First clean build after adding `noteRegistry.cpp` failed with `SERIAL` symbol conflict in `mcuProtocol.cpp`. Investigation showed the compiled `.cpp` already used `DEVICE_SERIAL` correctly — the error came from a stale `.pio/build` object file referencing an older version.
- **Fix:** Ran `pio run --target clean` to purge build cache, then full rebuild succeeded.
- **Files modified:** None (build system state only)
- **Commit:** No separate commit — build cache is not version-controlled

## Verification Results

All 7 checks from the plan passed:

1. `grep "LED_MAX_BRIGHTNESS" src/pinDefines.h` — constant defined as 180
2. `grep "LED_MAX_BRIGHTNESS" src/mcuButton.cpp` — used in setLedState (not hardcoded 255)
3. `grep "sendNoteOn" src/mcuButton.cpp` — found in read()
4. `grep "setLedState" src/mcuButton.cpp` — NOT inside read() (confirmed via body inspection)
5. `grep "noteToButton" src/noteRegistry.h` — map field present
6. `grep "nullptr" src/noteRegistry.cpp` — getButton returns nullptr on miss
7. `pio run` — build SUCCESS (RAM 2.9%, Flash 3.6%)

## Self-Check: PASSED

All files exist on disk. Both task commits verified in git history.
