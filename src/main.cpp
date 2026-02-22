#include <Arduino.h>
#include <SoftPWM.h>
#include "pinDefines.h"
#include "inputManager.h"
#include "mcuProtocol.h"

InputManager inputManager;

// BOOT-01: Cascade startup animation — lights all 24 LEDs in sequence then turns them off.
// Runs blocking in setup() BEFORE usbMIDI handlers are registered. Safe because no MIDI
// callbacks are active yet. Confirms all LED hardware works on every power-on.
// LED order: K1–K16 (grid, row by row), then P1–P8 (track buttons).
static void playStartupAnimation()
{
    const int gridLeds[]  = { LED_K1, LED_K2, LED_K3, LED_K4, LED_K5, LED_K6, LED_K7, LED_K8,
                               LED_K9, LED_K10, LED_K11, LED_K12, LED_K13, LED_K14, LED_K15, LED_K16 };
    const int trackLeds[] = { LED_P1, LED_P2, LED_P3, LED_P4, LED_P5, LED_P6, LED_P7, LED_P8 };

    // Light each grid LED in sequence
    for (int i = 0; i < 16; i++) {
        SoftPWMSet(gridLeds[i], LED_MAX_BRIGHTNESS);
        delay(40);
    }
    // Then each track LED
    for (int i = 0; i < 8; i++) {
        SoftPWMSet(trackLeds[i], LED_MAX_BRIGHTNESS);
        delay(40);
    }
    // Hold briefly so user sees all LEDs lit
    delay(150);
    // Fade all off
    for (int i = 0; i < 16; i++) SoftPWMSet(gridLeds[i], 0);
    for (int i = 0; i < 8; i++)  SoftPWMSet(trackLeds[i], 0);
    delay(300);  // allow SoftPWM fade to complete
}
// Total animation duration: 24 x 40ms + 150 + 300 = ~1.4 seconds

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
