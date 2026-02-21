# Codebase Concerns

**Analysis Date:** 2026-02-20

## Tech Debt

**Incomplete variable naming convention:**
- Issue: Inconsistent use of underscore prefix for member variables. The TODO comment in `src/button.cpp:40` indicates `ledState` should follow the `_ledState` pattern used elsewhere in the class.
- Files: `src/button.h`, `src/button.cpp`
- Impact: Reduces code consistency and makes it harder to distinguish member variables from local variables at a glance.
- Fix approach: Rename `ledState` to `_ledState` throughout Button class and update all references.

**Potentiometer::init() is empty:**
- Issue: `Potentiometer::init()` in `src/potentiometer.cpp` contains no initialization logic, though the method is declared.
- Files: `src/potentiometer.h`, `src/potentiometer.cpp`
- Impact: Suggests incomplete implementation. If pin mode or ADC configuration is needed, it's missing.
- Fix approach: Either remove the declaration/call to `init()` if not needed, or implement proper analog pin initialization (pinMode, ADC settings).

**Unused event handlers:**
- Issue: `handleStart()` and `handleClock()` in `src/main.cpp` (lines 33-40) only print debug messages and don't implement transport control logic.
- Files: `src/main.cpp`
- Impact: Transport synchronization features are non-functional; transport clock pulses and start/stop signals from the DAW are logged but ignored.
- Fix approach: Implement actual transport control handlers or remove them if not needed.

**Commented-out code in main.cpp:**
- Issue: Lines 18-26 in `src/main.cpp` contain commented Note-On/Note-Off handler stubs with incomplete comments.
- Files: `src/main.cpp`
- Impact: Dead code clutters the file and suggests incomplete feature exploration. Unclear intent.
- Fix approach: Remove or commit to implementing MIDI note messages.

**Commented-out fade time configuration:**
- Issue: Lines 6-7 in `src/inputManager.cpp` have commented SoftPWMSetFadeTime calls with two different values (100ms and 500ms).
- Files: `src/inputManager.cpp`
- Impact: LED fade timing is inconsistent across buttons. Current behavior uses default SoftPWM fade time (125ms per `src/button.cpp:8`), not the developer's intentions.
- Fix approach: Uncomment one fade time or establish a consistent LED fade strategy across all buttons.

**Inactive debug Serial output:**
- Issue: `handleControlChangeMessage()` in `src/main.cpp:29` prints "CONTROL CHANGE" to Serial on every CC message received.
- Files: `src/main.cpp`
- Impact: Excessive Serial output may impact timing performance in tight feedback loops between DAW and hardware. No way to disable debug output in production.
- Fix approach: Either remove debug prints or make them configurable (e.g., `#define DEBUG_CC 0`).

## Known Bugs

**Inverted MIDI CC values for button press detection:**
- Bug description: Button logic is inverted — when `ledState == LOW`, the code sends CC 127 (on); when `ledState == HIGH`, it sends CC 0 (off).
- Symptoms: Buttons send opposite values from what the visual state suggests. LED state `LOW` → sends on message (127), LED state `HIGH` → sends off message (0).
- Files: `src/button.cpp:20-26`
- Trigger: Any button press toggles `ledState` and sends the inverted CC value.
- Workaround: DAW must invert incoming button CC values to interpret them correctly, or user expects inverted behavior.

**Unsafe pointer dereferencing in InputManager initialization:**
- Bug description: In `src/inputManager.h:22-38` and `src/inputManager.h:40-48`, Button arrays are initialized by dereferencing pointers returned from `registerButton()`. The dereference happens at compile-time, but the underlying pointers could become invalid if ButtonRegistry is destroyed before InputManager.
- Symptoms: Memory corruption or segfault if ButtonRegistry is deallocated before the button arrays (unlikely in practice due to static initialization order, but fragile).
- Files: `src/inputManager.h:22-48`, `src/buttonRegistry.cpp`
- Trigger: Occurs at initialization; would only manifest if ButtonRegistry is dynamically destroyed.
- Workaround: Store Button pointers directly instead of dereferencing at initialization.

**setLedState uses copy instead of reference:**
- Bug description: In `src/inputManager.cpp:25`, a Button is created as a copy: `Button button = *(buttonRegistry.ccNumToButton[ccNum]);`. This creates a temporary copy, so calling `setLedState()` on the copy does not affect the original Button object.
- Symptoms: LEDs don't update in response to incoming CC messages from the DAW, even though the function is called.
- Files: `src/inputManager.cpp:25-26`
- Trigger: When DAW sends a CC message to any button, the LED should update but doesn't.
- Workaround: None. This is a critical bug in DAW feedback. LEDs will only update on local button presses.

## Security Considerations

**No input validation on MIDI CC numbers:**
- Risk: The `handleControlChangeMessage()` function checks channel (line 21) but only validates CC numbers for expected ranges. Invalid CC numbers could be accessed outside the grid/track button ranges, potentially accessing uninitialized memory or causing undefined behavior.
- Files: `src/inputManager.cpp:19-29`
- Current mitigation: Hardcoded CC ranges (102-117, 20-27) are only accessed if they match. Direct `map` access without bounds checking: `buttonRegistry.ccNumToButton[ccNum]` could throw `std::out_of_range` or access non-existent entries.
- Recommendations: Add explicit validation that `ccNum` exists in `ccNumToButton` map before dereferencing. Use `find()` or `count()` to check membership first.

**No protection against button index out-of-bounds in arrays:**
- Risk: Button and Potentiometer arrays are created in InputManager header file (static size) but there's no runtime validation that loop counters stay in bounds.
- Files: `src/inputManager.cpp:9-42`
- Current mitigation: Loop limits are hardcoded (`NUM_GRID_BUTTONS`, `NUM_TRACKS`) and match array sizes, so bounds are correct at compile-time.
- Recommendations: Consider using standard containers (std::array, std::vector) with bounds-checked access if behavior changes in future.

## Performance Bottlenecks

**Excessive MIDI message transmission from analog inputs:**
- Problem: Every call to `Potentiometer::read()` checks for a change using a 3-unit threshold (ANALOG_NOISE), but sends a CC message unconditionally if the threshold is exceeded. No debouncing or rate-limiting on analog inputs.
- Files: `src/potentiometer.cpp:7-24`, `src/potentiometer.h:18`
- Cause: ADC fluctuations near the threshold cause frequent CC messages even when the physical knob/slider hasn't moved. Teensy 3.5 running at ~72MHz should handle this, but unnecessary USB traffic wastes bandwidth and may lag the DAW feedback loop.
- Improvement path: Implement hysteresis (different thresholds for rising/falling) or aggregate changes into a ring buffer and only send if the average change exceeds a larger threshold. Consider debouncing potentiometers like buttons.

**SoftPWM overhead without clear benefit:**
- Problem: All button LEDs are controlled via SoftPWM with 125ms fade time, even though the fade is not visible at normal interaction speeds and consumes CPU cycles on a software PWM library instead of hardware PWM.
- Files: `src/button.cpp:8`, `src/button.h`
- Cause: SoftPWM is easier to use than configuring hardware PWM pins, but it's less efficient. Teensy 3.5 has hardware PWM available on many pins.
- Improvement path: Use hardware PWM for LED brightness control, or remove the fade effect if 0/255 brightness is sufficient. Measure CPU impact if fading is important for UX.

**InputManager::readAll() always polls all inputs:**
- Problem: Every main loop iteration calls `readAll()` which iterates all 16 grid buttons, 8 track buttons, 8 knobs, and 8 sliders (40 `read()` calls). Even if no input has changed, all are checked.
- Files: `src/inputManager.cpp:31-42`
- Cause: Naive full polling strategy. Reasonable for this hardware count, but scales poorly if inputs are added.
- Improvement path: If inputs are added, consider interrupt-driven input or at least skipping reads for inputs that haven't changed recently.

## Fragile Areas

**Button class state synchronization with DAW:**
- Files: `src/button.h`, `src/button.cpp`, `src/inputManager.cpp`
- Why fragile: Button has local `ledState` that can be set from two sources: local press (toggles state) and DAW feedback (sets state directly). If the user presses a button while the DAW is sending the same CC, the state may desynchronize. No bidirectional state verification exists.
- Safe modification: Before adding features like LED blink patterns or different feedback modes, establish a clear state ownership model: does the button always reflect the DAW state, or does it maintain local toggle state?
- Test coverage: No unit tests. Manual testing required to verify LED state matches button presses.

**ButtonRegistry pointer lifetime:**
- Files: `src/buttonRegistry.h`, `src/buttonRegistry.cpp`, `src/inputManager.h`
- Why fragile: `ButtonRegistry` allocates Button objects with `new` but never deletes them. The InputManager stores references via pointer dereference, creating a dependency on ButtonRegistry lifetime. If ButtonRegistry is ever destroyed, all Button references become invalid.
- Safe modification: Either adopt a clear object ownership model (static lifetimes, smart pointers like `std::unique_ptr`) or ensure ButtonRegistry is never destroyed.
- Test coverage: No tests verify pointer validity or memory cleanup.

**Pin definitions hardcoded in enumerations:**
- Files: `src/pinDefines.h:22-23`, hardware pin assignments throughout
- Why fragile: Two pins are flagged with concerning comments: `K15_SW` and `K16_SW` are connected to USB native ports (UD+/UD-), which are reserved pins on Teensy 3.5. Using these pins for input will interfere with USB functionality.
- Safe modification: Remap these buttons to available pins. Test USB MIDI stability if you add new features.
- Test coverage: No functional test verifies USB stability with current pin configuration.

## Scaling Limits

**Maximum 16 grid buttons + 8 track buttons:**
- Current capacity: 24 buttons (16 grid + 8 track), 16 knobs/sliders
- Limit: Teensy 3.5 has 64 GPIO pins. With 24 buttons (button + LED = 48 pins) + 16 analog inputs (knobs/sliders) = 64 pins. There's no room to add more inputs. Custom PCB is fully populated.
- Scaling path: Upgrade to Teensy 4.1 (more pins), use a pin multiplexer (shift registers), or implement LED matrix to reduce pin count (requires more complex driver code).

**No support for MIDI channels beyond channel 1:**
- Current capacity: All buttons/knobs/sliders use MIDI channel 1
- Limit: Most DAWs support 16 MIDI channels. Current firmware hardcodes channel 1.
- Scaling path: Modify Button and Potentiometer to accept configurable MIDI channels, then add a mode selector or configuration interface.

**No configuration persistence:**
- Current capacity: Firmware behavior is completely determined at compile-time (pin mappings, CC numbers, thresholds).
- Limit: Cannot change MIDI mappings or behavior without recompiling and re-uploading.
- Scaling path: Add an EEPROM configuration store so users can reprogram mappings via SYSEX or a companion tool without recompiling.

## Dependencies at Risk

**Bounce2 library maintenance:**
- Risk: Bounce2 is a well-maintained debouncing library, but any breaking changes in future versions could require code updates. Current pinned version is `^2.70` (allow up to 3.x).
- Impact: Button debouncing stops working if Bounce2 API changes.
- Migration plan: If Bounce2 becomes unmaintained, implement simple debouncing in-house or use an alternative library like OneButton. Current usage is simple (attach, interval, update, changed/fell).

**SoftPWM library stability:**
- Risk: SoftPWM (bhagman/SoftPWM @^1.0.1) is less actively maintained than Bounce2. Software PWM libraries are inherently timing-sensitive and may have issues on newer hardware or Arduino core versions.
- Impact: LED fading could stop working, flicker, or consume excessive CPU if the library breaks.
- Migration plan: Switch to hardware PWM using Teensy's built-in AnalogWrite or PWM functions. Requires changing `src/button.cpp:8` and `src/button.cpp:41` to use digitalWrite/analogWrite instead of SoftPWM functions.

**Teensy 3.5 platform deprecation:**
- Risk: Teensy 3.5 is largely deprecated (mentioned in README). PlatformIO support is stable, but the Arduino core may stop receiving updates.
- Impact: Future Arduino or Teensy core updates may break USB MIDI functionality.
- Migration plan: The firmware should be largely portable to Teensy 4.1 with pin remapping (mentioned in README). Plan a migration path if core support is discontinued.

## Test Coverage Gaps

**No unit tests:**
- What's not tested: Button debouncing, MIDI CC transmission, LED state updates, potentiometer filtering, ButtonRegistry lookups, control change message handling.
- Files: All source files in `src/`
- Risk: Bugs like the LED state copy-by-value in `src/inputManager.cpp:25` and inverted button logic in `src/button.cpp:20-26` could have been caught by basic unit tests.
- Priority: High — add at least smoke tests for Button, Potentiometer, and ButtonRegistry before adding new features.

**No integration tests with simulated DAW:**
- What's not tested: Bidirectional MIDI feedback loop (hardware → DAW → hardware LED update), full control flow from button press to LED feedback.
- Files: Entire firmware
- Risk: The LED feedback bug (`src/inputManager.cpp:25`) went unnoticed because there's no automated test that presses a button, simulates a DAW CC response, and verifies the LED updates.
- Priority: High — add integration test suite using Teensy simulator or mock MIDI input.

**No USB stability test:**
- What's not tested: USB enumeration with K15_SW and K16_SW connected to reserved pins, USB MIDI message throughput, behavior under rapid button presses or slider movements.
- Files: `src/pinDefines.h`, `src/main.cpp`
- Risk: USB interference from pins 22-23 could cause intermittent disconnects or data loss.
- Priority: Medium — test USB stability with current hardware configuration, plan pin migration if issues found.

**No analog input linearity test:**
- What's not tested: Potentiometer mapping accuracy, hysteresis behavior, noise filtering effectiveness across the full ADC range.
- Files: `src/potentiometer.cpp`
- Risk: If a knob or slider is damaged or the ADC is noisy, MIDI messages may be incorrect or erratic.
- Priority: Medium — add a test that reads analog values and verifies mapped CC output is linear and stable.

---

*Concerns audit: 2026-02-20*
