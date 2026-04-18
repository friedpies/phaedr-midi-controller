#include "inputManager.h"
#include "mcuConfig.h"

void InputManager::init()
{
    SoftPWMBegin();

    // ---- Grid buttons (K1–K16) ----
    // K1: Bank Left — shifts 8-channel bank window 8 tracks left
    gridButtons[0].setup(K1_SW,  LED_K1,  MCU_NOTE_BANK_LEFT,  DEBOUNCE_TIME);
    // K2: Bank Right — shifts 8-channel bank window 8 tracks right
    gridButtons[1].setup(K2_SW,  LED_K2,  MCU_NOTE_BANK_RIGHT, DEBOUNCE_TIME);
    gridButtons[2].setup(K3_SW,  LED_K3,  -1, DEBOUNCE_TIME);
    gridButtons[3].setup(K4_SW,  LED_K4,  -1, DEBOUNCE_TIME);
    gridButtons[4].setup(K5_SW,  LED_K5,  -1, DEBOUNCE_TIME);
    gridButtons[5].setup(K6_SW,  LED_K6,  -1, DEBOUNCE_TIME);
    gridButtons[6].setup(K7_SW,  LED_K7,  -1, DEBOUNCE_TIME);
    gridButtons[7].setup(K8_SW,  LED_K8,  -1, DEBOUNCE_TIME);
    gridButtons[8].setup(K9_SW,  LED_K9,  -1, DEBOUNCE_TIME);
    gridButtons[9].setup(K10_SW, LED_K10, -1, DEBOUNCE_TIME);
    gridButtons[10].setup(K11_SW, LED_K11, -1, DEBOUNCE_TIME);
    gridButtons[11].setup(K12_SW, LED_K12, -1, DEBOUNCE_TIME);

    // K13: Cycle (Loop) enable/disable — has LED
    gridButtons[12].setup(K13_SW, LED_K13, MCU_NOTE_LOOP, DEBOUNCE_TIME);

    // K14: Record — has LED
    gridButtons[13].setup(K14_SW, LED_K14, MCU_NOTE_RECORD, DEBOUNCE_TIME);

    // K15: Play — NO LED hardware (ledPin=-1)
    gridButtons[14].setup(K15_SW, -1, MCU_NOTE_PLAY, DEBOUNCE_TIME);

    // K16: Stop — NO LED hardware (ledPin=-1)
    gridButtons[15].setup(K16_SW, -1, MCU_NOTE_STOP, DEBOUNCE_TIME);

    // Note: Rewind (note 91) and FastForward (note 92) are NOT assigned.
    // This hardware has no physical Rewind or FF buttons — hardware design constraint.
    // MCU-04 is satisfied by Stop (93), Play (94), and Record (95).

    // ---- Track buttons (P1–P8) ----
    // Track selectors — LED shows local selection state; no MCU note sent
    trackButtons[0].setup(P1_SW, LED_P1, -1, DEBOUNCE_TIME);
    trackButtons[1].setup(P2_SW, LED_P2, -1, DEBOUNCE_TIME);
    trackButtons[2].setup(P3_SW, LED_P3, -1, DEBOUNCE_TIME);
    trackButtons[3].setup(P4_SW, LED_P4, -1, DEBOUNCE_TIME);
    trackButtons[4].setup(P5_SW, LED_P5, -1, DEBOUNCE_TIME);
    trackButtons[5].setup(P6_SW, LED_P6, -1, DEBOUNCE_TIME);
    trackButtons[6].setup(P7_SW, LED_P7, -1, DEBOUNCE_TIME);
    trackButtons[7].setup(P8_SW, LED_P8, -1, DEBOUNCE_TIME);

    // ---- Faders (SLIDE_1–SLIDE_8) ----
    // Pitch Bend on MIDI channels 1–8 per MCU fader protocol
    trackFaders[0].setup(SLIDE_1, 1);
    trackFaders[1].setup(SLIDE_2, 2);
    trackFaders[2].setup(SLIDE_3, 3);
    trackFaders[3].setup(SLIDE_4, 4);
    trackFaders[4].setup(SLIDE_5, 5);
    trackFaders[5].setup(SLIDE_6, 6);
    trackFaders[6].setup(SLIDE_7, 7);
    trackFaders[7].setup(SLIDE_8, 8);

    // PICK-03/PICK-04: Associate each fader with its channel strip button for blink control.
    // fader[0] (MIDI ch1, SLIDE_1) <-> trackButtons[0] (P1), ..., fader[7] <-> trackButtons[7] (P8)
    for (int i = 0; i < NUM_TRACKS; i++) {
        trackFaders[i].setChannelButton(&trackButtons[i]);
    }

    // ---- Init all buttons ----
    for (int i = 0; i < NUM_GRID_BUTTONS; i++) {
        gridButtons[i].init();
    }
    for (int i = 0; i < NUM_TRACKS; i++) {
        trackButtons[i].init();
    }

    // ---- NoteRegistry — register buttons that receive LED feedback from Logic ----
    // K1–K12: inert — not registered
    noteRegistry.registerButton(MCU_NOTE_LOOP,      &gridButtons[12]);   // K13 Cycle
    noteRegistry.registerButton(MCU_NOTE_PUNCH_IN,  &gridButtons[MCU_PUNCH_GRID_INDEX]);
    noteRegistry.registerButton(MCU_NOTE_METRONOME, &gridButtons[MCU_METRO_GRID_INDEX]);
    noteRegistry.registerButton(MCU_NOTE_RECORD,    &gridButtons[13]);   // K14 Record
    // K15 Play, K16 Stop: no LED hardware — not registered
    // P1–P8: SELECT notes 24–31 — LED driven by Logic's SELECT feedback
    for (int i = 0; i < NUM_TRACKS; i++) {
        noteRegistry.registerButton(MCU_NOTE_SELECT_BASE + i, &trackButtons[i]);
    }
}

// Physical 2D positions of each LED in grid-cell units (one grid cell = 1.0).
// K1–K14 have LEDs; K15–K16 do not (no LED hardware). P1–P8 have LEDs.
static const float LED_POS_X[22] = {
    0.f, 1.f, 2.f, 3.f,   // K1–K4   row 0
    0.f, 1.f, 2.f, 3.f,   // K5–K8   row 1
    0.f, 1.f, 2.f, 3.f,   // K9–K12  row 2
    0.f, 1.f,              // K13–K14 row 3 (K15–K16 have no LEDs)
    4.5f, 5.5f, 6.5f, 7.5f, 8.5f, 9.5f, 10.5f, 11.5f  // P1–P8
};
static const float LED_POS_Y[22] = {
    0.f, 0.f, 0.f, 0.f,   // K1–K4   row 0
    1.f, 1.f, 1.f, 1.f,   // K5–K8   row 1
    2.f, 2.f, 2.f, 2.f,   // K9–K12  row 2
    3.f, 3.f,              // K13–K14 row 3
    2.8f, 2.8f, 2.8f, 2.8f, 2.8f, 2.8f, 2.8f, 2.8f  // P1–P8
};

static const int   RIPPLE_N       = 22;
static const float RIPPLE_SPEED    = 90.0f;  // ms per grid-cell of distance
static const int   RIPPLE_HOLD_MS  = 260;    // how long each LED stays bright before fading
static const float RIPPLE_DECAY    = 0.35f;  // exponential decay rate — higher = steeper drop-off

static const int RIPPLE_LEDS[22] = {
    LED_K1, LED_K2, LED_K3, LED_K4, LED_K5, LED_K6, LED_K7, LED_K8,
    LED_K9, LED_K10, LED_K11, LED_K12, LED_K13, LED_K14,
    LED_P1, LED_P2, LED_P3, LED_P4, LED_P5, LED_P6, LED_P7, LED_P8
};

void InputManager::triggerRipple(int originIdx)
{
    for (int i = 0; i < RIPPLE_N; i++) SoftPWMSet(RIPPLE_LEDS[i], 0);

    float ox = LED_POS_X[originIdx];
    float oy = LED_POS_Y[originIdx];
    for (int i = 0; i < RIPPLE_N; i++) {
        float dx = LED_POS_X[i] - ox;
        float dy = LED_POS_Y[i] - oy;
        float d = sqrtf(dx * dx + dy * dy);
        _ripple.dist[i]       = d;
        _ripple.brightness[i] = (uint8_t)(LED_MAX_BRIGHTNESS * expf(-d * RIPPLE_DECAY));
        _ripple.lit[i]   = false;
        _ripple.faded[i] = false;
    }
    _ripple.startMs = millis();
    _ripple.active  = true;
}

static const int NUM_GRID_LEDS = 14;  // K1–K14 have LEDs; K15–K16 do not

void InputManager::readIdle()
{
    for (int i = 0; i < NUM_GRID_BUTTONS; i++) {
        // Only trigger ripple for buttons that have LEDs (K1–K14 = indices 0–13)
        if (gridButtons[i].poll() && i < NUM_GRID_LEDS) triggerRipple(i);
    }
    for (int i = 0; i < NUM_TRACKS; i++) {
        if (trackButtons[i].poll()) triggerRipple(NUM_GRID_LEDS + i);
    }
}

void InputManager::updateRipple()
{
    if (!_ripple.active) return;

    uint32_t elapsed = millis() - _ripple.startMs;
    bool     anyLeft = false;

    for (int i = 0; i < RIPPLE_N; i++) {
        if (_ripple.faded[i]) continue;
        anyLeft = true;

        uint32_t lightAt = (uint32_t)(_ripple.dist[i] * RIPPLE_SPEED);
        uint32_t fadeAt  = lightAt + (uint32_t)RIPPLE_HOLD_MS;

        if (!_ripple.lit[i] && elapsed >= lightAt) {
            SoftPWMSet(RIPPLE_LEDS[i], _ripple.brightness[i]);
            _ripple.lit[i] = true;
        }
        if (_ripple.lit[i] && elapsed >= fadeAt) {
            SoftPWMSet(RIPPLE_LEDS[i], 0);
            _ripple.faded[i] = true;
        }
    }

    if (!anyLeft) _ripple.active = false;
}

void InputManager::handleNoteMessage(byte note, uint8_t velocity)
{
    MCUButton* btn = noteRegistry.getButton(note);
    if (btn != nullptr) {
        btn->setLedState(velocity);
        // velocity 0 (from NoteOff or NoteOn+vel0) = LED off
        // velocity 127 = LED on
        // velocity 1 = blink (Phase 2)
    }
}

void InputManager::readAll()
{
    for (int i = 0; i < NUM_GRID_BUTTONS; i++) {
        gridButtons[i].read();
    }
    for (int i = 0; i < NUM_TRACKS; i++) {
        // P1–P7: SELECT Ch1–7. P8: SELECT Ch8 (master).
        // LED driven by Logic's SELECT feedback via NoteRegistry.
        if (trackButtons[i].poll()) {
            _selectedTrack = i;
            usbMIDI.sendNoteOn(MCU_NOTE_SELECT_BASE + i, 127, 1);
            usbMIDI.sendNoteOff(MCU_NOTE_SELECT_BASE + i, 0, 1);
        }
        bool knobMoved  = trackKnobs[i].read();
        bool faderMoved = trackFaders[i].read();

        if ((knobMoved || faderMoved) && _selectedTrack != i) {
            _selectedTrack = i;
            usbMIDI.sendNoteOn(MCU_NOTE_SELECT_BASE + i, 127, 1);
            usbMIDI.sendNoteOff(MCU_NOTE_SELECT_BASE + i, 0, 1);
        }
    }
}

void InputManager::updateBlinks()
{
    // PICK-03: advance blink state machines for all 8 channel strip buttons.
    // Must be called every loop() iteration — millis()-delta gating ensures no blocking.
    for (int i = 0; i < NUM_TRACKS; i++) {
        trackButtons[i].updateBlink();
    }
}

void InputManager::setFaderDawValue(int faderIdx, int value14bit)
{
    // PICK-01: route incoming pitch bend (from handlePitchBend in main.cpp) to the correct fader.
    // faderIdx is 0-based (caller passes channel - 1).
    if (faderIdx < 0 || faderIdx >= NUM_TRACKS) return;
    trackFaders[faderIdx].setDawValue(value14bit);
}

