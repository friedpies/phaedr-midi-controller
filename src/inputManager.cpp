#include "inputManager.h"
#include "mcuConfig.h"
#include "ledBudget.h"

void InputManager::init()
{
    SoftPWMBegin();

    // ---- Grid buttons (K1–K16) ----
    // Row 1 — session state (bidirectional: press toggles, LED mirrors Logic state)
    gridButtons[0].setup(K1_SW,  LED_K1,  MCU_NOTE_METRONOME, DEBOUNCE_TIME);
    gridButtons[1].setup(K2_SW,  LED_K2,  MCU_NOTE_PUNCH_IN,  DEBOUNCE_TIME);
    gridButtons[2].setup(K3_SW,  LED_K3,  MCU_NOTE_SAVE,      DEBOUNCE_TIME);
    gridButtons[3].setup(K4_SW,  LED_K4,  MCU_NOTE_UNDO,      DEBOUNCE_TIME);
    // Row 2 — user-assignable F1–F4
    gridButtons[4].setup(K5_SW,  LED_K5,  MCU_NOTE_F1, DEBOUNCE_TIME);
    gridButtons[5].setup(K6_SW,  LED_K6,  MCU_NOTE_F2, DEBOUNCE_TIME);
    gridButtons[6].setup(K7_SW,  LED_K7,  MCU_NOTE_F3, DEBOUNCE_TIME);
    gridButtons[7].setup(K8_SW,  LED_K8,  MCU_NOTE_F4, DEBOUNCE_TIME);
    // Row 3 — user-assignable F5–F8
    gridButtons[8].setup(K9_SW,  LED_K9,  MCU_NOTE_F5, DEBOUNCE_TIME);
    gridButtons[9].setup(K10_SW, LED_K10, MCU_NOTE_F6, DEBOUNCE_TIME);
    gridButtons[10].setup(K11_SW, LED_K11, MCU_NOTE_F7, DEBOUNCE_TIME);
    gridButtons[11].setup(K12_SW, LED_K12, MCU_NOTE_F8, DEBOUNCE_TIME);

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
    // Mute toggles — press sends Mute note, LED mirrors Logic mute state.
    // Fader/knob movement auto-focuses the track via SELECT (handled in readAll()).
    trackButtons[0].setup(P1_SW, LED_P1, MCU_NOTE_MUTE_BASE + 0, DEBOUNCE_TIME);
    trackButtons[1].setup(P2_SW, LED_P2, MCU_NOTE_MUTE_BASE + 1, DEBOUNCE_TIME);
    trackButtons[2].setup(P3_SW, LED_P3, MCU_NOTE_MUTE_BASE + 2, DEBOUNCE_TIME);
    trackButtons[3].setup(P4_SW, LED_P4, MCU_NOTE_MUTE_BASE + 3, DEBOUNCE_TIME);
    trackButtons[4].setup(P5_SW, LED_P5, MCU_NOTE_MUTE_BASE + 4, DEBOUNCE_TIME);
    trackButtons[5].setup(P6_SW, LED_P6, MCU_NOTE_MUTE_BASE + 5, DEBOUNCE_TIME);
    trackButtons[6].setup(P7_SW, LED_P7, MCU_NOTE_MUTE_BASE + 6, DEBOUNCE_TIME);
    trackButtons[7].setup(P8_SW, LED_P8, MCU_NOTE_MUTE_BASE + 7, DEBOUNCE_TIME);

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

    for (int i = 0; i < NUM_TRACKS; i++) {
        trackButtons[i].setInvertLed(true);
        trackButtons[i].setActive(false);
    }

    // Light-while-held for K1–K12 only. K13 (Cycle) and K14 (Record) receive Logic's
    // LED feedback, and K15/K16 (Play/Stop) have no LED hardware — all excluded.
    for (int i = 0; i < 12; i++) {
        gridButtons[i].setLightWhileHeld(true);
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
    // P1–P8: Mute notes 16–23 — LED driven by Logic's Mute feedback
    for (int i = 0; i < NUM_TRACKS; i++) {
        noteRegistry.registerButton(MCU_NOTE_MUTE_BASE + i, &trackButtons[i]);
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
    for (int i = 0; i < RIPPLE_N; i++) LedBudget::set(RIPPLE_LEDS[i], 0);

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
            LedBudget::set(RIPPLE_LEDS[i], _ripple.brightness[i]);
            _ripple.lit[i] = true;
        }
        if (_ripple.lit[i] && elapsed >= fadeAt) {
            LedBudget::set(RIPPLE_LEDS[i], 0);
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
        // P1–P8: Mute toggle — auto-sends mute note via read(), LED mirrors Logic feedback.
        trackButtons[i].read();

        bool knobMoved  = trackKnobs[i].read();
        bool faderMoved = trackFaders[i].read();

        // Seed the select anchor on first valid fader position, before any movement check.
        int faderPos = trackFaders[i].getLast14bit();
        if (faderPos >= 0 && _faderSelectAnchor[i] < 0) {
            _faderSelectAnchor[i] = faderPos;
        }

        // Select-intent gate: a fader move only counts if it has traveled beyond
        // FADER_SELECT_THRESHOLD since we last fired a SELECT on this fader.
        // Knob moves pass through unchanged — Potentiometer already filters at ~2% travel.
        bool faderSelectIntent = faderMoved && faderPos >= 0 &&
            abs(faderPos - _faderSelectAnchor[i]) > Fader::FADER_SELECT_THRESHOLD;

        if ((knobMoved || faderSelectIntent) && _selectedTrack != i) {
            _selectedTrack = i;
            if (faderSelectIntent) _faderSelectAnchor[i] = faderPos;
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
    trackButtons[faderIdx].setActive(true);
    trackFaders[faderIdx].setDawValue(value14bit);
}

