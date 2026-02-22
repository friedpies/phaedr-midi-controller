#include <Arduino.h>
#include <SoftPWM.h>
#include "pinDefines.h"
#include "inputManager.h"
#include "mcuProtocol.h"

InputManager inputManager;

// BOOT-01: Pulse startup animation — a window of ~5 LEDs sweeps back and forth like a wave.
// SoftPWM fade-out on the trailing edge creates a natural comet tail behind the pulse.
// Runs blocking in setup() BEFORE usbMIDI handlers are registered.
static void playStartupAnimation()
{
    const int allLeds[] = {
        LED_K1, LED_K2, LED_K3, LED_K4, LED_K5, LED_K6, LED_K7, LED_K8,
        LED_K9, LED_K10, LED_K11, LED_K12, LED_K13, LED_K14, LED_K15, LED_K16,
        LED_P1, LED_P2, LED_P3, LED_P4, LED_P5, LED_P6, LED_P7, LED_P8
    };
    const int N      = 24;
    const int STEP   = 35;  // ms per LED position
    const int WINDOW = 5;   // pulse width — trailing edge fades via SoftPWM

    // 3 full back-and-forth sweeps (~6.5 seconds total)
    for (int sweep = 0; sweep < 3; sweep++) {
        // Forward: pulse travels K1 → P8
        for (int i = 0; i < N; i++) {
            SoftPWMSet(allLeds[i], LED_MAX_BRIGHTNESS);
            if (i >= WINDOW) SoftPWMSet(allLeds[i - WINDOW], 0);
            delay(STEP);
        }
        // Fade out trailing window, pause before reversing
        for (int i = N - WINDOW; i < N; i++) SoftPWMSet(allLeds[i], 0);
        delay(150);

        // Backward: pulse travels P8 → K1
        for (int i = N - 1; i >= 0; i--) {
            SoftPWMSet(allLeds[i], LED_MAX_BRIGHTNESS);
            if (i + WINDOW < N) SoftPWMSet(allLeds[i + WINDOW], 0);
            delay(STEP);
        }
        // Fade out trailing window, pause before next sweep (or ending)
        for (int i = WINDOW - 1; i >= 0; i--) SoftPWMSet(allLeds[i], 0);
        delay(150);
    }
    delay(200);  // final settle
}
// Total: 3 × (24×35ms fwd + 150 + 24×35ms back + 150) + 200 ≈ 6.5 seconds

// SysEx handler — forwards to MCUProtocol state machine
void handleSysEx(const uint8_t* data, uint16_t length, bool complete)
{
    mcuProtocol.handleSysEx(data, length, complete);
}

// NoteOn from Logic — LED state feedback
// velocity 127 = LED on, 1 = blink (Phase 2), 0 = LED off
void handleNoteOn(byte channel, byte note, byte velocity)
{
    inputManager.handleNoteMessage(note, velocity);
}

// NoteOff from Logic — LED off
// Some Logic versions send NoteOff instead of NoteOn(vel=0) for LED-off state
void handleNoteOff(byte channel, byte note, byte velocity)
{
    inputManager.handleNoteMessage(note, 0);  // treat NoteOff as velocity 0
}

void setup()
{
    // SoftPWMBegin() is called inside inputManager.init() — do not call twice
    inputManager.init();
    playStartupAnimation();     // BOOT-01: cascade animation confirms LEDs work on power-on
    mcuProtocol.begin();

    usbMIDI.setHandleSystemExclusive(handleSysEx);
    usbMIDI.setHandleNoteOn(handleNoteOn);
    usbMIDI.setHandleNoteOff(handleNoteOff);
    // Note: CC handler intentionally removed — per CONTEXT.md clean break decision
    // Note: handleStart/handleClock/handleStop will be added in Phase 4 for beat chaser
}

void loop()
{
    inputManager.readAll();
    mcuProtocol.update();  // handles handshake retry timer
    usbMIDI.read();
}
