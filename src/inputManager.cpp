#include "inputManager.h"

void InputManager::init()
{
    SoftPWMBegin();

    // ---- Grid buttons (K1–K16) ----
    // K1–K8: MUTE notes 16–23 (Phase 1 placeholder — Phase 3 assigns final notes)
    gridButtons[0].setup(K1_SW,  LED_K1,  16, DEBOUNCE_TIME);   // MUTE Ch1
    gridButtons[1].setup(K2_SW,  LED_K2,  17, DEBOUNCE_TIME);   // MUTE Ch2
    gridButtons[2].setup(K3_SW,  LED_K3,  18, DEBOUNCE_TIME);   // MUTE Ch3
    gridButtons[3].setup(K4_SW,  LED_K4,  19, DEBOUNCE_TIME);   // MUTE Ch4
    gridButtons[4].setup(K5_SW,  LED_K5,  20, DEBOUNCE_TIME);   // MUTE Ch5
    gridButtons[5].setup(K6_SW,  LED_K6,  21, DEBOUNCE_TIME);   // MUTE Ch6
    gridButtons[6].setup(K7_SW,  LED_K7,  22, DEBOUNCE_TIME);   // MUTE Ch7
    gridButtons[7].setup(K8_SW,  LED_K8,  23, DEBOUNCE_TIME);   // MUTE Ch8

    // K9–K12: SELECT notes 24–27 (Phase 1 placeholder — Phase 3 assigns final notes)
    gridButtons[8].setup(K9_SW,   LED_K9,  24, DEBOUNCE_TIME);  // SELECT Ch1
    gridButtons[9].setup(K10_SW,  LED_K10, 25, DEBOUNCE_TIME);  // SELECT Ch2
    gridButtons[10].setup(K11_SW, LED_K11, 26, DEBOUNCE_TIME);  // SELECT Ch3
    gridButtons[11].setup(K12_SW, LED_K12, 27, DEBOUNCE_TIME);  // SELECT Ch4

    // K13: Shift — unmapped (Phase 3 feature); ledPin=-1 and noteNum=-1 = fully inert
    // MCUButton::read() guards against noteNum < 0 — no MIDI output when pressed.
    // MCUButton::init() guards against buttonPin... but K13_SW pin is valid hardware.
    // Using noteNum=-1 ensures no accidental MIDI note is sent (note 0 = Ch1 REC).
    gridButtons[12].setup(K13_SW, -1, -1, DEBOUNCE_TIME);       // Shift — unmapped Phase 3

    // K14: Record — MCU-04 REQUIRED, note 95, has LED
    gridButtons[13].setup(K14_SW, LED_K14, 95, DEBOUNCE_TIME);  // Record (MCU-04)

    // K15: Stop — MCU-04 REQUIRED, note 93, NO LED hardware (ledPin=-1)
    gridButtons[14].setup(K15_SW, -1, 93, DEBOUNCE_TIME);       // Stop (MCU-04), no LED

    // K16: Play — MCU-04 REQUIRED, note 94, NO LED hardware (ledPin=-1)
    gridButtons[15].setup(K16_SW, -1, 94, DEBOUNCE_TIME);       // Play (MCU-04), no LED

    // Note: Rewind (note 91) and FastForward (note 92) are NOT assigned.
    // This hardware has no physical Rewind or FF buttons — hardware design constraint.
    // MCU-04 is satisfied by Stop (93), Play (94), and Record (95).

    // ---- Track buttons (P1–P8) ----
    // MCU REC notes 0–7: Ch1 REC = note 0, Ch8 REC = note 7
    trackButtons[0].setup(P1_SW, LED_P1, 0, DEBOUNCE_TIME);
    trackButtons[1].setup(P2_SW, LED_P2, 1, DEBOUNCE_TIME);
    trackButtons[2].setup(P3_SW, LED_P3, 2, DEBOUNCE_TIME);
    trackButtons[3].setup(P4_SW, LED_P4, 3, DEBOUNCE_TIME);
    trackButtons[4].setup(P5_SW, LED_P5, 4, DEBOUNCE_TIME);
    trackButtons[5].setup(P6_SW, LED_P6, 5, DEBOUNCE_TIME);
    trackButtons[6].setup(P7_SW, LED_P7, 6, DEBOUNCE_TIME);
    trackButtons[7].setup(P8_SW, LED_P8, 7, DEBOUNCE_TIME);

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

    // ---- Init all buttons ----
    for (int i = 0; i < NUM_GRID_BUTTONS; i++) {
        gridButtons[i].init();
    }
    for (int i = 0; i < NUM_TRACKS; i++) {
        trackButtons[i].init();
    }

    // K13 (Shift) has ledPin=-1 so MCUButton::init() skips SoftPWM registration for it.
    // But LED_K13 (pin 29) is real hardware that appears in the startup animation.
    // Register it explicitly here so SoftPWMSet(LED_K13, ...) works during animation.
    // K13 has no MCU note feedback in Phase 1, so it is not added to NoteRegistry.
    SoftPWMSet(LED_K13, 0);
    SoftPWMSetFadeTime(LED_K13, 125, 125);

    // ---- NoteRegistry — register all buttons with LEDs ----
    // Grid buttons K1–K12: MUTE/SELECT placeholders (have LEDs)
    noteRegistry.registerButton(16, &gridButtons[0]);   // K1  MUTE Ch1
    noteRegistry.registerButton(17, &gridButtons[1]);   // K2  MUTE Ch2
    noteRegistry.registerButton(18, &gridButtons[2]);   // K3  MUTE Ch3
    noteRegistry.registerButton(19, &gridButtons[3]);   // K4  MUTE Ch4
    noteRegistry.registerButton(20, &gridButtons[4]);   // K5  MUTE Ch5
    noteRegistry.registerButton(21, &gridButtons[5]);   // K6  MUTE Ch6
    noteRegistry.registerButton(22, &gridButtons[6]);   // K7  MUTE Ch7
    noteRegistry.registerButton(23, &gridButtons[7]);   // K8  MUTE Ch8
    noteRegistry.registerButton(24, &gridButtons[8]);   // K9  SELECT Ch1
    noteRegistry.registerButton(25, &gridButtons[9]);   // K10 SELECT Ch2
    noteRegistry.registerButton(26, &gridButtons[10]);  // K11 SELECT Ch3
    noteRegistry.registerButton(27, &gridButtons[11]);  // K12 SELECT Ch4
    // K13 (index 12): unmapped — not registered (Shift, Phase 3)
    noteRegistry.registerButton(95, &gridButtons[13]);  // K14 Record (MCU-04)
    // K15 (index 14): Stop — not registered (no LED hardware)
    // K16 (index 15): Play — not registered (no LED hardware)

    // Track buttons P1–P8: MCU REC notes 0–7
    noteRegistry.registerButton(0, &trackButtons[0]);
    noteRegistry.registerButton(1, &trackButtons[1]);
    noteRegistry.registerButton(2, &trackButtons[2]);
    noteRegistry.registerButton(3, &trackButtons[3]);
    noteRegistry.registerButton(4, &trackButtons[4]);
    noteRegistry.registerButton(5, &trackButtons[5]);
    noteRegistry.registerButton(6, &trackButtons[6]);
    noteRegistry.registerButton(7, &trackButtons[7]);
}

// LED positions matching the boot animation order: K1–K16 then P1–P8
static const int RIPPLE_LEDS[] = {
    LED_K1, LED_K2, LED_K3, LED_K4, LED_K5, LED_K6, LED_K7, LED_K8,
    LED_K9, LED_K10, LED_K11, LED_K12, LED_K13, LED_K14, LED_K15, LED_K16,
    LED_P1, LED_P2, LED_P3, LED_P4, LED_P5, LED_P6, LED_P7, LED_P8
};
static const int RIPPLE_N       = 24;
static const int RIPPLE_STEP_MS = 40;  // ms between each LED position advancing
static const int RIPPLE_WINDOW  = 5;   // how many positions stay lit before the tail fades

void InputManager::readIdle()
{
    // Poll grid buttons — LED indices 0-15
    for (int i = 0; i < NUM_GRID_BUTTONS; i++) {
        if (gridButtons[i].poll()) {
            for (int j = 0; j < RIPPLE_N; j++) SoftPWMSet(RIPPLE_LEDS[j], 0);
            _ripple.active   = true;
            _ripple.origin   = i;
            _ripple.startMs  = millis();
            _ripple.lastStep = -1;
        }
    }
    // Poll track buttons — LED indices 16-23
    for (int i = 0; i < NUM_TRACKS; i++) {
        if (trackButtons[i].poll()) {
            for (int j = 0; j < RIPPLE_N; j++) SoftPWMSet(RIPPLE_LEDS[j], 0);
            _ripple.active   = true;
            _ripple.origin   = NUM_GRID_BUTTONS + i;
            _ripple.startMs  = millis();
            _ripple.lastStep = -1;
        }
    }
}

void InputManager::updateRipple()
{
    if (!_ripple.active) return;

    int step = (int)((millis() - _ripple.startMs) / RIPPLE_STEP_MS);
    if (step == _ripple.lastStep) return;  // nothing new this loop
    _ripple.lastStep = step;

    // Light wavefront: LEDs at exactly `step` positions from origin, spreading both ways
    int ahead  = _ripple.origin + step;
    int behind = _ripple.origin - step;
    if (ahead < RIPPLE_N)                    SoftPWMSet(RIPPLE_LEDS[ahead],  LED_MAX_BRIGHTNESS);
    if (behind >= 0 && behind != ahead)      SoftPWMSet(RIPPLE_LEDS[behind], LED_MAX_BRIGHTNESS);

    // Fade tail: LEDs WINDOW positions behind the wavefront (SoftPWM fade handles the glow-off)
    int fadeAhead  = ahead  - RIPPLE_WINDOW;
    int fadeBehind = behind + RIPPLE_WINDOW;
    if (fadeAhead >= 0 && fadeAhead < RIPPLE_N)                          SoftPWMSet(RIPPLE_LEDS[fadeAhead],  0);
    if (fadeBehind >= 0 && fadeBehind < RIPPLE_N && fadeBehind != fadeAhead) SoftPWMSet(RIPPLE_LEDS[fadeBehind], 0);

    // Done when wavefront has cleared all 24 positions and tail has fully faded
    int maxDist = max(_ripple.origin, RIPPLE_N - 1 - _ripple.origin);
    if (step > maxDist + RIPPLE_WINDOW) {
        _ripple.active = false;
    }
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
        trackButtons[i].read();
        trackKnobs[i].read();
        trackFaders[i].read();
    }
}
