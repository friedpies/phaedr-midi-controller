#include <Arduino.h>
#include <SoftPWM.h>
#include "pinDefines.h"
#include "inputManager.h"
#include "mcuProtocol.h"

InputManager inputManager;

// BOOT-01: Slosh startup animation — wave fills K1→K16→P1→P8, drains back, repeats twice.
// Runs blocking in setup() BEFORE usbMIDI handlers are registered. Safe because no MIDI
// callbacks are active yet. Confirms all LED hardware works on every power-on.
static void playStartupAnimation()
{
    const int allLeds[] = {
        LED_K1, LED_K2, LED_K3, LED_K4, LED_K5, LED_K6, LED_K7, LED_K8,
        LED_K9, LED_K10, LED_K11, LED_K12, LED_K13, LED_K14, LED_K15, LED_K16,
        LED_P1, LED_P2, LED_P3, LED_P4, LED_P5, LED_P6, LED_P7, LED_P8
    };
    const int N = 24;
    const int STEP_MS = 35;  // 35ms per LED step

    // 2 full sloshes: fill forward → drain backward → fill forward → drain backward
    for (int slosh = 0; slosh < 2; slosh++) {
        for (int i = 0; i < N; i++) {
            SoftPWMSet(allLeds[i], LED_MAX_BRIGHTNESS);
            delay(STEP_MS);
        }
        delay(200);  // hold with all LEDs lit
        for (int i = N - 1; i >= 0; i--) {
            SoftPWMSet(allLeds[i], 0);
            delay(STEP_MS);
        }
        delay(200);  // hold empty (allow SoftPWM fade to settle)
    }
}
// Total duration: 2 × (24×35ms fill + 200ms hold + 24×35ms drain + 200ms hold) ≈ 4.2 seconds

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
