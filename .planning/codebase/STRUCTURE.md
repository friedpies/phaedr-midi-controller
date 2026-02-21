# Codebase Structure

**Analysis Date:** 2026-02-20

## Directory Layout

```
phaedr-midi-controller/
├── src/                    # Main firmware source code
│   ├── main.cpp           # Entry point: setup() and loop()
│   ├── inputManager.h     # Input orchestration header
│   ├── inputManager.cpp   # Input orchestration implementation
│   ├── button.h           # Button abstraction header
│   ├── button.cpp         # Button abstraction implementation
│   ├── potentiometer.h    # Potentiometer abstraction header
│   ├── potentiometer.cpp  # Potentiometer abstraction implementation
│   ├── buttonRegistry.h   # CC-to-Button lookup registry header
│   ├── buttonRegistry.cpp # CC-to-Button lookup registry implementation
│   └── pinDefines.h       # Hardware pin constants
├── include/               # External library includes (managed by PlatformIO)
├── lib/                   # Project-specific libraries (empty)
├── test/                  # Test directory (unused)
├── assets/                # Hardware assets (STLs, images)
├── platformio.ini         # PlatformIO build configuration
├── README.md              # Project documentation
└── CLAUDE.md              # Developer instructions (this file)
```

## Directory Purposes

**src/ (Main Source):**
- Purpose: All firmware C++ source and header files
- Contains: Input abstractions, orchestration, hardware configuration
- Key files: `main.cpp` (entry), `inputManager.h/cpp` (orchestrator), `button.h/cpp`, `potentiometer.h/cpp`

**include/ (External Includes):**
- Purpose: Placeholder for external library headers (managed by PlatformIO)
- Contains: Arduino core, MIDIUSB library headers (symlinked by PlatformIO)
- Key files: None modified by user

**lib/ (Local Libraries):**
- Purpose: Project-specific libraries (currently empty)
- Contains: Nothing; external dependencies managed via platformio.ini
- Key files: None

**test/ (Tests):**
- Purpose: Test directory
- Contains: Empty; no unit tests implemented
- Key files: None

**assets/ (Hardware Assets):**
- Purpose: STL files, hardware documentation, images
- Contains: 3D models for enclosure/mounting
- Key files: None affecting firmware

## Key File Locations

**Entry Points:**
- `src/main.cpp`: Arduino sketch entry point containing `setup()` (initialization) and `loop()` (main loop)

**Configuration:**
- `src/pinDefines.h`: All hardware pin assignments (switches, LEDs, analog inputs)
- `platformio.ini`: Build environment, dependencies, compiler flags

**Core Logic:**
- `src/inputManager.h/cpp`: Central orchestrator for all 40 inputs (buttons, knobs, sliders)
- `src/button.h/cpp`: Button abstraction with debouncing and LED control
- `src/potentiometer.h/cpp`: Analog input abstraction with noise filtering

**Input Registry:**
- `src/buttonRegistry.h/cpp`: CC number → Button pointer lookup table

## Naming Conventions

**Files:**
- `.cpp` / `.h` pairs: One public interface header (.h) per class/module, implementation in .cpp
- Example: `button.h` (interface) / `button.cpp` (implementation)
- Lowercase with camelCase for multi-word names: `inputManager.h`, `buttonRegistry.h`, `pinDefines.h`

**Functions:**
- Camel case: `readAll()`, `handleControlChangeMessage()`, `registerButton()`, `setLedState()`
- Public methods: typically verbs describing action (`init()`, `read()`, `setLedState()`)
- Private methods: helper operations (`ledStateToPWM()`, `hasChanged()`)

**Variables:**
- Private member variables: Prefixed with underscore and camelCase: `_buttonPin`, `_ccNum`, `_debounceTime`, `_invert`
- Public member state: camelCase without underscore: `ledState`, `lastReading`
- Constants: SCREAMING_SNAKE_CASE: `NUM_GRID_BUTTONS`, `DEBOUNCE_TIME`, `ANALOG_NOISE`, `READ_RESOLUTION`

**Types:**
- Classes: PascalCase: `Button`, `Potentiometer`, `InputManager`, `ButtonRegistry`
- Macros: SCREAMING_SNAKE_CASE for pin defines: `K1_SW`, `LED_K1`, `SLIDE_1`, `KNOB_1`

**Pin Name Pattern:**
- Grid buttons: `K{1-16}_SW` (switch), `LED_K{1-16}` (LED)
- Track buttons: `P{1-8}_SW` (switch), `LED_P{1-8}` (LED)
- Knobs: `KNOB_{1-8}` (analog input pin)
- Sliders: `SLIDE_{1-8}` (analog input pin)

## Where to Add New Code

**New Feature (e.g., new input type, new MIDI handler):**
- Primary code: `src/` directory, follow existing class structure (header/implementation pair)
- Example: To add rotary encoder input, create `rotaryEncoder.h/cpp` following Button/Potentiometer pattern
- Registration: Instantiate in InputManager (either as array or single instance, see `src/inputManager.h` lines 22–60)
- Testing: No test framework in place; manual testing via serial monitor and DAW

**New Component/Module (e.g., display feedback, bootloader update):**
- Implementation: Create new `src/moduleName.h/cpp` pair
- Integration point: Usually in `src/main.cpp` or within InputManager depending on scope
- Dependencies: Add to `platformio.ini` lib_deps if external library required

**Utilities (Shared Helpers):**
- Shared helpers: `src/utils/` (not yet created; would follow same naming as existing modules)
- Currently: No utilities directory; all helpers are inline or embedded in main classes

## Special Directories

**include/ (PlatformIO-Managed):**
- Purpose: Symlinked external headers for Arduino core and libraries
- Generated: Yes (by PlatformIO on build)
- Committed: No; ignored by .gitignore

**lib/ (Local Dependencies):**
- Purpose: Vendored local libraries (if any)
- Generated: No
- Committed: Yes (but currently empty)

**assets/ (Hardware):**
- Purpose: STL files, schematics, hardware reference
- Generated: No (created by CAD software)
- Committed: Yes (via Git LFS for large STL files)

**.git/ (Version Control):**
- Purpose: Git repository metadata
- Generated: Yes
- Committed: No (ignored)

## Mapping: CC Numbers to Hardware

| Input Type | Count | CC Range | File Reference |
|---|---|---|---|
| Grid buttons (rows 1–4 × 4 columns) | 16 | 102–117 | `src/inputManager.h` lines 22–38 |
| Track buttons (P1–P8) | 8 | 20–27 | `src/inputManager.h` lines 40–48 |
| Knobs (inverted) | 8 | 14–15, 28–31, 118–119 | `src/inputManager.h` line 50 |
| Sliders | 8 | 3, 9, 85–90 | `src/inputManager.h` lines 52–60 |

All messages on MIDI channel 1. See `src/pinDefines.h` for switch/LED pin assignments.

## Common Navigation Patterns

**To find where a button's LED is updated:**
1. Start in `src/inputManager.cpp` line 19–28: `handleControlChangeMessage()` routes incoming CC
2. Follow `buttonRegistry.ccNumToButton[ccNum]` to ButtonRegistry: `src/buttonRegistry.h/cpp`
3. ButtonRegistry was populated in InputManager `init()` via `registerButton()` calls
4. Button pointer then calls `setLedState()`: `src/button.cpp` line 38–42

**To find hardware pin for a switch:**
1. Open `src/pinDefines.h`
2. Search for the pin constant (e.g., `K1_SW` for grid button 1, or `P1_SW` for track button 1)
3. Cross-reference LED pin (e.g., `LED_K1` for grid button 1 LED)

**To trace analog input (knob/slider) reading:**
1. Start in `src/inputManager.cpp` line 40–42: `readAll()` calls `trackKnobs[i].read()` and `trackSliders[i].read()`
2. Jump to `src/potentiometer.cpp` line 7–24: `read()` polls analog pin, filters noise, sends CC
3. CC number defined in Potentiometer constructor call in `src/inputManager.h` lines 50, 52–60

---

*Structure analysis: 2026-02-20*
