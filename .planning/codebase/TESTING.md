# Testing Patterns

**Analysis Date:** 2026-02-20

## Test Framework

**Runner:**
- PlatformIO Unit Testing framework (optional, not configured)
- Test directory structure exists at `test/` but no actual test implementations present
- Config: `test/README` indicates PlatformIO unit testing support is available but not in use

**Assertion Library:**
- Not applicable — no tests implemented

**Run Commands:**
```bash
pio test                    # Run PlatformIO unit tests (if configured)
pio run                     # Build firmware (current workflow)
pio run --target upload     # Upload to Teensy (validation method)
pio device monitor          # Serial monitor for manual testing
```

## Test File Organization

**Location:**
- Dedicated `test/` directory exists at repository root (`/Users/kenmarut/repo/phaedr-midi-controller/test/`)
- Currently empty except for `README`
- Convention: PlatformIO expects test files in `test/test_*/` subdirectories

**Naming:**
- Not established — no test files present

**Structure:**
```
test/
└── README                  # PlatformIO unit testing documentation
```

## Current Testing Approach

**No Automated Tests:**
- The codebase has no unit tests, integration tests, or automated test suites
- Testing is entirely manual and hardware-based

**Manual Testing Workflow:**
1. Build firmware: `pio run`
2. Upload to Teensy: `pio run --target upload`
3. Monitor serial output: `pio device monitor`
4. Physical interaction: press buttons, turn knobs, move sliders
5. Observe: LEDs light up, Serial output logs events, DAW receives/responds to MIDI

**Validation Strategy:**
- Hardware integration testing: buttons send correct CC values (102–117 for grid, 20–27 for track buttons)
- LED feedback testing: DAW sends CC values that correctly update LED states
- Analog input testing: knobs and sliders send smoothed, mapped CC values (0–127)
- Bidirectional MIDI testing: controller ↔ DAW CC message round-trips

## Test Structure

**Serial Logging for Verification:**
Observable patterns in code that support manual testing:

```cpp
// main.cpp - MIDI event logging
void handleControlChangeMessage(byte channel, byte ccNum, byte velocity)
{
    Serial.println("CONTROL CHANGE");
    inputManager.handleControlChangeMessage(channel, ccNum, velocity);
}

void handleStart()
{
    Serial.println("HANDLE START");
}

void handleClock()
{
    Serial.println("HANDLE CLOCK");
}
```

**Commented Debug Code (Disabled Tests):**
- `button.cpp` has commented-out handlers for NoteOn/NoteOff events
- `potentiometer.cpp` has conditional debug output:
  ```cpp
  // if (_pin == SLIDE_1)
  // {
  //     Serial.println(lastReading);
  // }
  ```
- These indicate previous testing hooks that could be re-enabled for debugging

## What Would Be Testable

**Unit Test Candidates:**
- `Potentiometer::hasChanged()` — pure function determining noise threshold (2 lines, easy to test)
  ```cpp
  bool Potentiometer::hasChanged(int newValue)
  {
      return (newValue >= (lastReading + ANALOG_NOISE) ||
              newValue <= (lastReading - ANALOG_NOISE));
  }
  ```

- `Button::ledStateToPWM()` — state-to-PWM mapping
  ```cpp
  int Button::ledStateToPWM(bool state)
  {
      return state ? 255 : 0;
  }
  ```

**Integration Test Candidates:**
- Button press → CC message emission: `Button::read()` with Bounce2 state change
- Analog read → CC mapping: `Potentiometer::read()` with ADC simulation
- CC receipt → LED update: `InputManager::handleControlChangeMessage()` → `Button::setLedState()`
- Registry lookup: `ButtonRegistry::registerButton()` and CC-to-button mapping

**Hardware-in-Loop Test Candidates:**
- 16 grid buttons emit correct CC numbers (102–117) on press
- 8 track buttons emit correct CC numbers (20–27) on press
- 8 knobs emit CC values 14–15, 28–31, 118–119 with inversion
- 8 sliders emit CC values 3, 9, 85–90 without inversion
- LED brightness responds to CC 127 (on) / CC 0 (off) from DAW
- Noise threshold prevents spurious CC messages from noisy analog readings

## Mocking Considerations

**Framework:** No mocking framework in use

**What Would Need Mocking:**
- Arduino hardware APIs: `pinMode()`, `digitalWrite()`, `analogRead()`
- Bounce2 debouncer: `Bounce::attach()`, `Bounce::update()`, `Bounce::changed()`, `Bounce::fell()`
- SoftPWM library: `SoftPWMSet()`, `SoftPWMBegin()`, `SoftPWMSetFadeTime()`
- MIDI USB library: `usbMIDI.sendControlChange()`, `usbMIDI.read()`, `usbMIDI.setHandle*()`

**Hardware Simulation Approach:**
- Create mock implementations of Bounce2 and SoftPWM
- Inject mock MIDI interface into InputManager
- Example structure for a button test:
  ```cpp
  // Hypothetical test (not implemented)
  void test_button_press_sends_cc() {
      MockBounce mockButton;
      mockButton.simulateFall(); // Press detected
      button.read(); // Should send CC
      assert(usbMIDI.lastCCNum == expectedCC);
      assert(usbMIDI.lastVelocity == 127);
  }
  ```

## Coverage Gaps

**Untested Code:**
- All of `Button::read()` — core button press logic (CC sending)
- All of `Potentiometer::read()` — analog reading and MIDI sending
- `InputManager::readAll()` — main loop orchestration
- `InputManager::handleControlChangeMessage()` — DAW-to-LED feedback
- `ButtonRegistry::registerButton()` — object lifecycle and map insertion
- `InputManager::init()` — hardware initialization sequence
- Hardware pin configuration in `pinDefines.h` — cannot test without hardware

**Risk Level:** HIGH
- Core MIDI communication path untested
- Button state management untested
- Analog filtering (noise threshold) untested but visible
- LED feedback loop untested

## Future Testing Path

**If Unit Tests Were Added:**

1. Extract pure functions from I/O-dependent code:
   - Debounce logic
   - Analog noise filtering (`hasChanged()`)
   - Value mapping (10-bit ADC → 7-bit MIDI)
   - State-to-PWM conversion

2. Create test helpers:
   - Mock Arduino environment (pins, digital I/O, analog I/O)
   - Mock Bounce2 button state machine
   - Mock SoftPWM brightness control
   - Mock MIDI USB interface

3. Write integration tests with mocks:
   - Button press flow: hardware → debounce → CC emission
   - Analog flow: ADC reading → noise filter → mapping → CC emission
   - LED feedback: CC receipt → lookup → brightness update

4. Establish CI/CD with PlatformIO:
   ```bash
   pio test --project-dir . --verbose
   ```

---

*Testing analysis: 2026-02-20*
