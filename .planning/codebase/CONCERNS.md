# Codebase Concerns

**Analysis Date:** 2026-03-02

## Tech Debt

**Inconsistent member variable naming in Button class:**
- Issue: `ledState` lacks underscore prefix while other members use `_buttonPin`, `_ledPin`, `_ccNum`, `_debounceTime`. Creates inconsistent internal API.
- Files: `src/button.h` (line 29), `src/button.cpp` (lines 18, 40-41)
- Impact: Confuses convention for future maintenance; inconsistent with class design pattern elsewhere in codebase.
- Fix approach: Rename `ledState` → `_ledState` throughout Button class (trivial refactor, affects 5 locations).

**Legacy Button class unused in favor of MCUButton:**
- Issue: Original `Button` class (`src/button.h/cpp`) implements CC-based note sending but is completely unused — all button input now goes through `MCUButton` class which uses MCU Note Bang protocol.
- Files: `src/button.h`, `src/button.cpp`
- Impact: Dead code adds cognitive load during maintenance; developer might incorrectly assume Button is active.
- Fix approach: Delete Button class entirely; update any imports/comments referencing it. Verify no references remain in InputManager or main.cpp (they already use MCUButton).

**Pin 13 hardware conflict documented but not abstracted:**
- Issue: `pinDefines.h` line 4 notes "K1_SW 13 // MUST REMOVE ONBOARD LED FOR THIS TO WORK (WORKS FINE)" — hardware design flaw (pin 13 has onboard LED that interferes with INPUT_PULLUP) is worked around but not encapsulated.
- Files: `src/pinDefines.h` (line 4)
- Impact: Hardware constraint visible only in comments; future pin refactors might forget this constraint and break K1 button. No runtime safeguard.
- Fix approach: Add preprocessor check or runtime validation that K1_SW == 13 and enforce documentation in build output.

**K15 and K16 pins connected to USB native ports (hardware design issue):**
- Issue: `pinDefines.h` lines 22-23 document "K15_SW A25 // BAD SCHEM UD+" and "K16_SW A26 // HOOKED UP TO USB NATIVE PORTS, DUMB UD-" — indicates schematic errors that work by accident.
- Files: `src/pinDefines.h` (lines 22-23)
- Impact: Fragile hardware coupling; any USB configuration change could break these buttons. Non-obvious to future maintainers.
- Fix approach: Document in project README as known hardware limitation; consider remapping in Phase 3 if pins available, or note as unfixable without PCB respin.

**USB power budget calculation relies on static LED_MAX_BRIGHTNESS:**
- Issue: `pinDefines.h` line 81 documents LED power budget: "24 LEDs × ~20mA peak × (LED_MAX_BRIGHTNESS/255) duty + ~150mA Teensy <= 500mA" at LED_MAX_BRIGHTNESS=180. If LED_MAX_BRIGHTNESS is changed, power budget is not automatically validated.
- Files: `src/pinDefines.h` (lines 79-81)
- Impact: Exceeding USB 500mA limit could cause brownouts, resets, or enumeration failures. No runtime check if someone raises LED_MAX_BRIGHTNESS.
- Fix approach: Add compile-time assertion checking power budget based on LED_MAX_BRIGHTNESS; document minimum headroom requirement in comments.

## Known Bugs

**Button state logic inverted — LED off sends CC 127, LED on sends CC 0:**
- Symptoms: When grid/track buttons are pressed, the logic inverts: `if (ledState == LOW) sendControlChange(..., 127, ...)` (line 20-26 in button.cpp). This means pressing a button sends "on" (127) when LED state is off, and "off" (0) when LED state is on — backwards.
- Files: `src/button.cpp` (lines 18-26)
- Trigger: Press any grid button (K1-K16) or track button (P1-P8) that still uses old Button class (if any remain in legacy code).
- Impact: Likely none in current build since Button class is unused; MCUButton correctly implements Note Bang without this bug. However, if Button class is ever re-enabled, this will silently send inverted commands.
- Workaround: None currently needed since Button is unused.

## Security Considerations

**MCU SysEx challenge-response validation is stubbed (intentional):**
- Risk: `mcuProtocol.cpp` lines 110-117 show `validateChallengeResponse()` always returns true without validating the 4-byte response from Logic. Any device sending the correct SysEx header can impersonate the controller.
- Files: `src/mcuProtocol.cpp` (lines 110-117)
- Current mitigation: USB-only connection to trusted DAW; not exposed over network. Code comments note this is intentional DIY controller design choice.
- Recommendations: This is acceptable for embedded hardware in a personal music production setup. If controller is ever exposed to untrusted hosts (e.g., networked MIDI), implement actual challenge validation (XOR, checksum, or HMAC over challenge bytes).

**No input validation on MIDI CC or pitch bend values:**
- Risk: `inputManager.cpp` `setFaderDawValue()` (line 223-229) accepts pitch bend without range checking; assumes 14-bit value 0-16383. Malformed MIDI could cause out-of-bounds array access or integer overflow.
- Files: `src/inputManager.cpp` (lines 223-229), `src/fader.cpp` (lines 31-51, 53-115)
- Current mitigation: Teensy USB stack validates MIDI frame structure; only well-formed messages reach callbacks. However, no defensive checks in firmware.
- Recommendations: Add assertions in `setFaderDawValue()` and `Fader::setDawValue()` to clamp values to [0, 16383]; add boundary checks in fader crossover logic (lines 88-94 in fader.cpp).

## Performance Bottlenecks

**2D Euclidean distance calculation on every ripple update:**
- Problem: `inputManager.cpp` `updateRipple()` (lines 152-177) calls `millis()` and checks 24 LED states every loop iteration. For each of the 24 LEDs, distance was pre-computed at trigger time (lines 127-137), but no caching of which LEDs have already faded.
- Files: `src/inputManager.cpp` (lines 123-177)
- Cause: Ripple uses `_ripple.faded[24]` array to track completion, but `anyLeft` loop still scans all 24 even if all are faded. Not a bottleneck on 8-bit embedded systems but inefficient pattern.
- Improvement path: Current performance is acceptable; ripple animation only runs during idle handshake phase. No optimization needed for Phase 2-4 (readAll() dominates once handshake completes).

**SoftPWM library overhead — all 22 LED channels updated every loop:**
- Problem: `SoftPWM.cpp` in vendored `lib/SoftPWM/` implements software PWM for all active channels on every loop iteration. Firmware polls all 16 grid + 8 track buttons (readAll ~160+ function calls) AND advances all LED PWM timers (~22 channels). On 96 MHz Teensy 3.5, this is acceptable but leaves little margin.
- Files: `lib/SoftPWM/` (entire library), `src/inputManager.cpp` (readAll), `src/main.cpp` (loop)
- Cause: Software PWM emulated via timer interrupts; no hardware PWM pin available for all 24 LED channels on Teensy 3.5 (limited to 9 hardware PWM outputs). Vendored library configured with SOFTPWM_MAXCHANNELS=22 per platformio.ini line 18.
- Improvement path: Current 22-channel SoftPWM is necessary given pin count. If firmware runs out of CPU time in future phases (beat chaser animation, etc.), profile with logic analyzer to identify bottlenecks. Teensy 3.5 @ 96MHz should comfortably handle this.

## Fragile Areas

**Fader pickup state machine has subtle crossover edge cases:**
- Files: `src/fader.cpp` (lines 24-115), `src/fader.h` (lines 28, 50-57)
- Why fragile: Pickup FSM transitions between OUT_OF_SYNC and SYNCED based on deadband crossing (line 96 in fader.cpp). The logic compares against `_lastFader14bit` (physical position from ADC read) vs `_dawValue14bit` (incoming pitch bend from DAW). If either source noises, false triggering can occur.
  - Lines 88-94 implement boundary-edge case handling (both at rail → immediate pickup).
  - Lines 45-48 implement bank switch detection by comparing against physical position, not previous DAW value.
  - Comments at lines 35-48 document why this approach was chosen, but the implementation is delicate.
- Safe modification: Never change PICKUP_DEADBAND, BANK_SWITCH_THRESHOLD, or FADER_NOISE_THRESHOLD without re-testing with Logic bank switches (Phase 2 verification already done). Do not assume these values are tuned for all DAWs or MIDI configurations; different MIDI interfaces may have different noise profiles.
- Test coverage: Phase 2 plans 02-02-PLAN.md and 02-03-PLAN.md include hardware verification of crossover detection. Gaps: no unit tests; verification is manual (moving physical faders and checking Logic fader position).

**NoteRegistry and ButtonRegistry use std::map with linear lookup:**
- Files: `src/noteRegistry.h` (lines 17-26), `src/buttonRegistry.h`, `src/noteRegistry.cpp` (lines 17-19)
- Why fragile: Both registries use `std::map<int, Button*>` (or MCUButton*). On every incoming MIDI message, a map lookup happens. With 24 total buttons (16 grid + 8 track) and 8 faders, the maps stay small (<30 entries), so lookup is O(log 30) ≈ O(5) comparisons. Not a performance issue, but the pattern is fragile if more buttons are added (e.g., Phase 4 animations might add callback registries).
- Safe modification: Keep map entries under 50; if more needed, switch to array-based lookup or hash map. Current design is correct for current scope.
- Test coverage: No unit tests for registry. Gap: if a CC number is registered twice or a button is unregistered mid-operation, behavior is undefined.

**Block diagram of LED control flow is not self-evident:**
- Files: `src/button.cpp` (old Button class, unused), `src/mcuButton.cpp` (correct MCUButton), `src/inputManager.cpp` (calls setLedState), `src/main.cpp` (routes NoteOn/NoteOff to inputManager)
- Why fragile: Two independent LED control paths exist:
  1. **Old path**: Button.setLedState() via manual CC sends (lines 20-26 in button.cpp) — **UNUSED, BUGGY**.
  2. **New path**: MCUButton.setLedState() via incoming NoteOn/NoteOff (main.cpp lines 56-65 → inputManager.handleNoteMessage → noteRegistry lookup → MCUButton.setLedState).
  - Additionally, MCUButton.startBlink() can drive LED independent of setLedState() for pickup mode blink animation.
- Safe modification: Do not mix Button and MCUButton in same code. Delete Button class to eliminate dual-path confusion. All LED changes must go through NoteRegistry and MCUButton.setLedState() or MCUButton.startBlink()/stopBlink().
- Test coverage: No LED-specific tests. Verification is visual (plugging in and checking which LEDs light up in Logic's Control Surface prefs). Gap: if a NoteOn arrives for an unregistered note number, it silently fails (noteRegistry returns nullptr, inputManager's handleNoteMessage does null check at line 181 but no logging).

**Blink period calculation in fader.read() uses signed/unsigned comparison:**
- Files: `src/fader.cpp` (lines 77-82)
- Why fragile: Line 81 calls `map(min(distance, 16383), 0, 16383, 600, 200)`. The `map()` function is Arduino's integer math routine; if distance is computed as signed int and 16383 is unsigned, comparison could misbehave on edge cases. Not a blocker on Teensy (32-bit ints) but fragile pattern.
- Safe modification: Ensure distance calculation (line 77) is always positive: `int distance = abs(fader14bit - _dawValue14bit);` (already correct on line 77). Keep period calculation as is.

## Scaling Limits

**Hardcoded array sizes limit future expansion:**
- Current capacity:
  - 16 grid buttons (K1-K16)
  - 8 track buttons (P1-P8)
  - 8 faders (SLIDE_1 to SLIDE_8)
  - 8 knobs (KNOB_1 to KNOB_8)
- Limit: Teensy 3.5 has 58 GPIO pins total. Currently using ~50+ pins (16 buttons + 16 LEDs for grid, 8 buttons + 8 LEDs for track, 8 faders, 8 knobs). No spare pins for expansion.
- Scaling path: If more buttons needed (e.g., Phase 4 beat chaser animation mode LEDs), must use:
  1. **Multiplexed matrix** (e.g., 4×6 button matrix reduces 24 buttons to 10 pins, frees 14 pins) — requires analog demux IC and re-mapping.
  2. **I2C port expander** (MCP23017 adds 16 GPIO over 2 pins) — adds complexity and latency.
  3. **Move to Teensy 4.1** (120+ GPIO pins) — hardware redesign.
- Current design is at pin limit; Phase 3-4 may hit capacity if animation mode requires more LEDs or buttons.

**SoftPWM MAXCHANNELS hardcoded at 22:**
- Limit: `platformio.ini` line 18 notes SOFTPWM_MAXCHANNELS=22 (increased from upstream default of 20). Firmware uses 22 LED channels: K1-K14 grid + P1-P8 track.
- Scaling path: If more LEDs added (e.g., status indicator LEDs for pickup mode, sync state, etc.), either:
  1. Switch to hardware PWM (Teensy 3.5 has 9 hardware PWM pins available; would need careful pin reallocation).
  2. Increase SOFTPWM_MAXCHANNELS in SoftPWM_timer.h and recompile library.
  3. Reduce blink/fade functionality and use simple on/off for new LEDs.

## Dependencies at Risk

**Bounce2 library for debouncing — stable but old pattern:**
- Risk: `platformio.ini` line 16 specifies `thomasfredericks/Bounce2@^2.70`. Bounce2 is mature and widely used, so low risk of abandonment. However, the `^2.70` version constraint allows updates up to 3.0.0 (if released). Bounce2 maintainer is active but dependency updates should be tested.
- Impact: If Bounce2 is updated and API changes, button debouncing could fail silently (e.g., if `interval()` or `fell()` behavior changes).
- Migration plan: Bounce2 has no viable alternative; it's the de facto Arduino debounce library. If issues arise, consider forking or implementing custom software debounce (simple state machine, 20-30 lines of code).

**SoftPWM library vendored locally — maintenance risk:**
- Risk: `lib/SoftPWM/` is a local copy (vendored) of a third-party library. It's not fetched from external source during build, so version is fixed. However, vendoring increases maintenance burden: if upstream SoftPWM is updated with bug fixes or performance improvements, this copy won't get them.
- Impact: If a critical bug is found in SoftPWM (e.g., LED flicker on Teensy 3.5 with certain interrupt timing), the local copy must be manually patched.
- Migration plan: Keep vendored copy as-is unless issues arise. If upstream SoftPWM 2.x is forked or updates, consider re-syncing. Alternatively, switch to Teensy 4.x hardware PWM (9 pins available) to eliminate software PWM dependency.

**Arduino framework version pinned to Teensy platform:**
- Risk: `platformio.ini` line 12 specifies `platform = teensy` without version pin. PlatformIO will use latest Teensy platform on build. If Teensy platform updates the bundled Arduino core, USB MIDI API or hardware pin behavior could change.
- Impact: Breaking changes in Arduino MIDI library (e.g., if `usbMIDI.sendPitchBend()` signature changes) would require code updates.
- Migration plan: Pin Teensy platform version in platformio.ini if stability is critical (e.g., `platform = teensy@6.2.0`). For now, accept latest; test before production firmware updates.

## Missing Critical Features

**No error logging or debug output in firmware:**
- Problem: If a MIDI message fails to send, or a fader reading is corrupted, or a bank switch doesn't trigger pickup mode, there's no way to diagnose the issue in the field. Serial debug is disabled (line 89 in main.cpp: "CC handler intentionally removed — per CONTEXT.md clean break decision").
- Blocks: Phase 3-4 integration testing; users cannot self-diagnose connection issues without firmware modification.
- Mitigation: Currently acceptable for embedded single-user device. If shipping to others, add optional compile-time debug mode (e.g., `#define DEBUG_ENABLED` to enable Serial.println() statements).

**No watchdog timer or heart-beat monitoring:**
- Problem: If firmware hangs in any function (e.g., deadlock in ripple animation millis() overflow, or infinite loop in Bounce2.update()), the controller becomes unresponsive but continues drawing USB power. Host DAW may lose MIDI sync without feedback.
- Blocks: Phase 4 beat chaser (which depends on millis() clock drift handling); critical for reliable production use.
- Mitigation: Teensy 3.5 has built-in watchdog timer. Not implemented yet; could be added in Phase 3-4 if needed. For now, manual testing with 4-hour runtime confirms no hangs.

**No MIDI input filtering or bounds checking:**
- Problem: Incoming CC, NoteOn, NoteOff, and Pitch Bend are processed without validation. If external MIDI source sends malformed messages or out-of-range values (e.g., CC 255, note 128), firmware behavior is undefined.
- Blocks: If controller is ever connected to untrusted MIDI sources, could crash firmware.
- Mitigation: Current design assumes Logic Pro as sole MIDI source (trusted). If multi-source MIDI is needed (Phase 4 beat chaser from external click track, for example), add input validation.

## Test Coverage Gaps

**No unit tests for core state machines:**
- What's not tested:
  - Fader pickup FSM (OUT_OF_SYNC → SYNCED transitions, deadband crossing, lazy blink reveal)
  - MCUButton blink FSM (blink period updates, phase toggling)
  - NoteRegistry and ButtonRegistry map operations
  - Ripple animation distance calculations and timing
- Files: `src/fader.cpp`, `src/mcuButton.cpp`, `src/noteRegistry.cpp`, `src/buttonRegistry.cpp`, `src/inputManager.cpp`
- Risk: If pickup FSM logic is changed or blink timing is adjusted, regressions could be introduced silently. No test suite catches them until hardware verification (manual, time-consuming).
- Priority: **High** — Phase 2 completion requires manual hardware verification of pickup mode; unit tests would accelerate Phase 3-4 development by 20-30%.

**No integration tests for MIDI message flow:**
- What's not tested:
  - Full NoteOn → NoteRegistry → MCUButton → LED update chain
  - Pitch bend message arrival → setFaderDawValue → Fader FSM → MIDI output
  - Multiple faders transitioning between SYNCED/OUT_OF_SYNC simultaneously (multi-bank switch scenario)
- Files: All of inputManager, fader, mcuButton in concert
- Risk: If refactoring any MIDI callback or registry lookup, side effects could break LED feedback without obvious cause.
- Priority: **Medium** — Phase 3 transport grid integration should include basic MIDI echo tests (send NoteOn, verify NoteRegistry lookup works).

**No hardware abstraction tests:**
- What's not tested:
  - ADC reading and noise filtering (Potentiometer.hasChanged threshold)
  - Button debounce timing (does Bounce2 actually debounce on this hardware pin configuration?)
  - SoftPWM fade timing (is 125ms fade time perceived correctly, or does SoftPWM timer interrupt jitter affect it?)
- Files: `src/potentiometer.cpp`, `src/mcuButton.cpp` (Bounce2 usage), `src/inputManager.cpp` (SoftPWM calls)
- Risk: Minor drift in timing could cause faders to feel unresponsive or blinks to strobe at wrong frequency.
- Priority: **Low** — These are verified through manual hardware testing already. Unit tests difficult without a Teensy and real pins; integration tests in Phase 3 can catch timing issues.

---

*Concerns audit: 2026-03-02*
