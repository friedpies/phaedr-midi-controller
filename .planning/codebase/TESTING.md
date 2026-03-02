# Testing Patterns

**Analysis Date:** 2026-03-02

## Test Framework

**Runner:**
- PlatformIO Unit Testing framework (configured in `platformio.ini`)
- Supported via `pio test` command
- Test directory: `test/`
- No actual test files found in repository (only README placeholder)

**Assertion Library:**
- Not determined (no test files present to inspect)
- PlatformIO supports Arduino Unit Test library by default

**Run Commands:**
```bash
pio test                  # Run all tests
pio test --verbose        # Run tests with detailed output
pio run --target upload   # Upload firmware to Teensy (deployment, not testing)
pio run                   # Build firmware
```

## Test File Organization

**Location:**
- Intended location: `test/` directory per PlatformIO standard
- Pattern: Not yet established (directory empty except for README)
- Recommended: `test/test_[module].cpp` per PlatformIO convention

**Naming:**
- Not yet established in this codebase
- Expected PlatformIO pattern: `test_*.cpp` files in `test/` directory

**Structure:**
```
test/
├── README              # PlatformIO template docs
└── (no test files)
```

## Current Testing Status

**Test Coverage:** Zero

**Reason:** Embedded firmware project with hardware dependencies (Teensy 3.5, SoftPWM, Bounce2 debouncing). Testing strategy not yet implemented.

**Obstacles to Testing:**
1. **Hardware dependencies:** Code depends on:
   - `analogRead()` (Teensy ADC)
   - `usbMIDI.sendControlChange()` (USB MIDI library)
   - `usbMIDI.sendNoteOn/Off()` (USB MIDI)
   - `SoftPWMSet()` (SoftPWM LED driver)
   - `Bounce::update()` (button debouncing library)
   - `millis()` (hardware timer)

2. **No abstraction layer:** Hardware calls are embedded directly in business logic:
   ```cpp
   // In Button::read() — direct MIDIUSB call
   usbMIDI.sendControlChange(_ccNum, 127, 1);

   // In Potentiometer::read() — direct analogRead
   int value = analogRead(_pin);
   ```

3. **Complex state:** Pickup mode (fader state synchronization) and ripple animations involve timing and spatial calculations that would need mocking.

## Recommended Testing Approach

### Unit Tests (if refactored for testability)

**Structure:**
```cpp
void test_potentiometer_absolute_mode() {
    // Would test: relative vs absolute MIDI sending
    // Requires: Mock analogRead(), mock usbMIDI
}

void test_fader_pickup_detection() {
    // Would test: bank switch detection logic
    // Requires: Mock Fader::read() to provide ADC values
}

void test_button_debounce_read() {
    // Would test: button state transitions
    // Requires: Mock Bounce2 library
}
```

**Mocking Pattern (if implemented):**
```cpp
// Pseudo-example — would need implementation
class MockMIDI {
    static std::vector<ControlChangeMessage> sentCC;
    static void sendControlChange(int cc, int value, int channel) {
        sentCC.push_back({cc, value, channel});
    }
};

// In test:
MockMIDI::sentCC.clear();
button.read();  // requires dependency injection of usbMIDI → MockMIDI
ASSERT_EQUAL(MockMIDI::sentCC[0].cc, 102);
```

### Integration Tests (firmware on hardware)

**Manual approach (current practice):**
1. Upload firmware to Teensy 3.5
2. Connect USB to computer running DAW (Logic Pro)
3. Test physical hardware:
   - Press buttons → verify LED feedback from DAW
   - Move faders → verify Pitch Bend CC messages sent
   - Turn knobs → verify relative MIDI CC messages sent
   - Verify startup animation runs
   - Verify MCU handshake completes

**No automated integration test framework detected.**

### Hardware Verification Checklist

Current testing is manual hardware verification (from phase SUMMARY):
- MCU handshake completes successfully
- LED pickup mode blink works
- Fader pickup synchronization works
- Bank switching enters pickup mode
- Startup animation cascades correctly
- Button debouncing suppresses noise

## Code Inspection for Testability Issues

**Non-Testable Patterns (present in codebase):**

1. **Global state in singletons:**
   ```cpp
   InputManager inputManager;  // global, single instance in main.cpp
   MCUProtocol mcuProtocol;    // global singleton
   ```
   → Difficult to reset state between tests

2. **Direct hardware calls in business logic:**
   ```cpp
   // Potentiometer::read() — cannot test without Teensy hardware
   int value = analogRead(_pin);
   ```

3. **Timing-dependent behavior:**
   ```cpp
   // MCUButton::updateBlink() — depends on millis()
   uint32_t now = millis();
   if (now - _lastBlinkMs >= halfPeriod) {
       // blink phase change
   }
   ```
   → Requires mocking of `millis()` or hardware clock

4. **Tightly-coupled objects:**
   ```cpp
   // Fader requires MCUButton* for blink control
   void Fader::setChannelButton(MCUButton* btn);
   ```
   → Cannot test Fader pickup logic without MCUButton instance

## Improving Testability (Future Recommendation)

**Dependency Injection Pattern:**
```cpp
// Current: direct call
int value = analogRead(_pin);

// Better: inject ADC reader
class Potentiometer {
    ADCReader* _adc;  // inject
    bool read() {
        int value = _adc->read(_pin);  // mockable
    }
};
```

**Hardware Abstraction Layer:**
```cpp
// Create interface
class MIDIOutput {
    virtual void sendCC(int cc, int value, int channel) = 0;
};

// Real implementation
class TeensyMIDI : public MIDIOutput {
    void sendCC(int cc, int value, int channel) override {
        usbMIDI.sendControlChange(cc, value, channel);
    }
};

// Testable version
class MockMIDI : public MIDIOutput {
    void sendCC(int cc, int value, int channel) override {
        sentMessages.push_back({cc, value, channel});
    }
};

// Use in Button
Button::Button(MIDIOutput* midi) : _midi(midi) { }
bool Button::read() {
    _midi->sendCC(_ccNum, 127, 1);  // now mockable
}
```

## Static Verification

**Manual Code Review:**
- Design reviews use git commits and phase planning documents
- No automated linting or static analysis tools detected
- Manual verification of:
  - MIDI CC number ranges (0-127 for controllers, 0-16383 for 14-bit)
  - ADC mapping: 10-bit input (0-1023) → 7-bit MIDI (0-127) or 14-bit (0-16383)
  - Pin configurations in `pinDefines.h`
  - Debounce timing (20ms standard)

## Testing Documentation

**Design documents by phase:**
- `CONTEXT.md` — MCU protocol design decisions
- Phase SUMMARYs in `.planning/phases/` — manual test results
- Comments in code (e.g., `PICK-01:`, `MCU-06:`) — design rationale

**No automated test documentation generated.**

## Notes on Embedded Testing Challenges

This codebase represents a typical embedded firmware project where:

1. **Hardware coupling is unavoidable** — testing must happen on real Teensy 3.5 or comprehensive mocking
2. **Timing is critical** — blink animations, debounce windows, ripple effects all time-dependent
3. **MIDI protocol** — USB MIDI communication requires actual USB connection or protocol simulation
4. **Resource constraints** — Arduino framework has limited memory for test harnesses

**Recommended test strategy for future phases:**
- Focus on hardware verification (manual testing on Teensy) for critical features
- Add debug serial output to trace state during testing (currently commented out)
- Use compile-time asserts for constant validation (`static_assert`)
- Document manual test procedures in phase planning instead of automated tests

---

*Testing analysis: 2026-03-02*
