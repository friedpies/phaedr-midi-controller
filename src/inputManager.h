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
    void readIdle();      // idle mode: debounce buttons, trigger ripple on press — no MIDI output
    void updateRipple();  // advance ripple animation one step (non-blocking, millis-based)
    void handleNoteMessage(byte note, uint8_t velocity);
    void updateBlinks();                               // PICK-03: drive all channel button blink timers
    void setFaderDawValue(int faderIdx, int val14bit); // PICK-01: route incoming pitch bend to fader

    static const int NUM_GRID_BUTTONS = 16;
    static const int NUM_TRACKS = 8;
    static const int DEBOUNCE_TIME = 20;

    NoteRegistry noteRegistry;

private:
    // 2D ripple state — per-LED distances computed at trigger time, checked each update
    // 22 LEDs: K1–K14 (14 grid LEDs, K15–K16 have no LEDs) + P1–P8 (8 track LEDs)
    struct RippleState {
        bool     active    = false;
        uint32_t startMs   = 0;
        float    dist[22];
        uint8_t  brightness[22];
        bool     lit[22];
        bool     faded[22];
    } _ripple;

    int8_t _selectedTrack = -1;  // -1 = none; 0–6 = tracks 1–7; 7 = master (P8)

    // Per-fader anchor: position at the last time this fader fired a SELECT.
    // A fader move only counts as "intent to select" if it travels more than
    // FADER_SELECT_THRESHOLD from this anchor — prevents jitter/hand-rest from
    // spuriously grabbing focus. -1 = uninitialized; seeded on first valid read.
    int _faderSelectAnchor[NUM_TRACKS] = { -1, -1, -1, -1, -1, -1, -1, -1 };

    void triggerRipple(int originIdx);  // compute distances, reset state, clear LEDs

    // MCUButton arrays — default-constructed here, configured in init() via setup()
    MCUButton gridButtons[NUM_GRID_BUTTONS];   // K1–K16
    MCUButton trackButtons[NUM_TRACKS];         // P1–P8: MCU SELECT notes 24–31

    // Fader array — default-constructed here, configured in init() via setup()
    // Sends Pitch Bend on MIDI channels 1–8 per MCU fader protocol
    Fader trackFaders[NUM_TRACKS];

    // MCU-06: Knobs on CC 16–23, relative VPot encoder format.
    // Logic Pro interprets CC 16-23 as relative encoder messages (MCU spec), not absolute
    // positions. Each message encodes CW (+, 0x01-0x3F) or CCW (-, 0x41-0x7F) step count.
    Potentiometer trackKnobs[NUM_TRACKS] = {
        Potentiometer(KNOB_1, 16, false, true),
        Potentiometer(KNOB_2, 17, false, true),
        Potentiometer(KNOB_3, 18, false, true),
        Potentiometer(KNOB_4, 19, false, true),
        Potentiometer(KNOB_5, 20, false, true),
        Potentiometer(KNOB_6, 21, false, true),
        Potentiometer(KNOB_7, 22, false, true),
        Potentiometer(KNOB_8, 23, false, true)
    };
};

#endif
