# Deferred Items — Phase 01: MCU Protocol Foundation

## Out-of-Scope Issues Discovered During Plan 03 Execution

### SERIAL name collision in mcuProtocol.h (from Plan 02)

**Discovered during:** Plan 03 Task 1 (build verification)
**File:** src/mcuProtocol.h line 34
**Issue:** `static const uint8_t SERIAL[7]` collides with Teensyduino `wiring.h` macro `#define SERIAL 0`. Causes compiler error:
```
/Users/kenmarut/.platformio/packages/framework-arduinoteensy/cores/teensy3/wiring.h:129:17: error: expected unqualified-id before numeric constant
 #define SERIAL  0
```
**Fix needed:** Rename `SERIAL` member in `mcuProtocol.h` to something that doesn't clash — e.g., `DEVICE_SERIAL` or `SERIAL_BYTES`.
**Scope:** Plan 02 artifact — should be addressed in a follow-up or Plan 05 when mcuProtocol is integrated.
