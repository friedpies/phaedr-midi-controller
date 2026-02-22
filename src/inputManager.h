#ifndef button_manager
#define button_manager

#include <Arduino.h>
#include "mcuButton.h"
#include "fader.h"
#include "noteRegistry.h"
#include "potentiometer.h"
#include "pinDefines.h"

// MCU-06: Knob CC numbers (was: CC 14, 15, 28, 29, 30, 31, 118, 119)
//         Now: CC 16, 17, 18, 19, 20, 21, 22, 23 per MCU VPot absolute format
class InputManager
{
public:
    void init();
    void readAll();
    void handleNoteMessage(byte note, uint8_t velocity);

    static const int NUM_GRID_BUTTONS = 16;
    static const int NUM_TRACKS = 8;
    static const int DEBOUNCE_TIME = 20;

    NoteRegistry noteRegistry;

    // MCUButton arrays — default-constructed here, configured in init() via setup()
    MCUButton gridButtons[NUM_GRID_BUTTONS];   // K1–K16
    MCUButton trackButtons[NUM_TRACKS];         // P1–P8: MCU REC notes 0–7

    // Fader array — default-constructed here, configured in init() via setup()
    // Sends Pitch Bend on MIDI channels 1–8 per MCU fader protocol
    Fader trackFaders[NUM_TRACKS];

    // MCU-06: Knobs reassigned to CC 16–23 (absolute VPot format)
    Potentiometer trackKnobs[NUM_TRACKS] = {
        Potentiometer(KNOB_1, 16, true),
        Potentiometer(KNOB_2, 17, true),
        Potentiometer(KNOB_3, 18, true),
        Potentiometer(KNOB_4, 19, true),
        Potentiometer(KNOB_5, 20, true),
        Potentiometer(KNOB_6, 21, true),
        Potentiometer(KNOB_7, 22, true),
        Potentiometer(KNOB_8, 23, true)
    };
};

#endif
