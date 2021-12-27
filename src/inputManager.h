#ifndef button_manager
#define button_manager

#include <Arduino.h>
#include "button.h"
#include "pinDefines.h"
#include "potentiometer.h"
#include "buttonRegistry.h"

class InputManager
{
public:
    void init();
    void readAll();
    void handleControlChangeMessage(byte channel, byte ccNum, byte velocity);
    static const int NUM_GRID_BUTTONS = 16;
    static const int NUM_TRACKS = 8;
    static const int DEBOUNCE_TIME = 20;
    ButtonRegistry buttonRegistry = ButtonRegistry();

    Button gridButtons[NUM_GRID_BUTTONS] = {
        *(buttonRegistry.registerButton(K1_SW, LED_K1, 102, DEBOUNCE_TIME)),
        *(buttonRegistry.registerButton(K2_SW, LED_K2, 103, DEBOUNCE_TIME)),
        *(buttonRegistry.registerButton(K3_SW, LED_K3, 104, DEBOUNCE_TIME)),
        *(buttonRegistry.registerButton(K4_SW, LED_K4, 105, DEBOUNCE_TIME)),
        *(buttonRegistry.registerButton(K5_SW, LED_K5, 106, DEBOUNCE_TIME)),
        *(buttonRegistry.registerButton(K6_SW, LED_K6, 107, DEBOUNCE_TIME)),
        *(buttonRegistry.registerButton(K7_SW, LED_K7, 108, DEBOUNCE_TIME)),
        *(buttonRegistry.registerButton(K8_SW, LED_K8, 109, DEBOUNCE_TIME)),
        *(buttonRegistry.registerButton(K9_SW, LED_K9, 110, DEBOUNCE_TIME)),
        *(buttonRegistry.registerButton(K10_SW, LED_K10, 111, DEBOUNCE_TIME)),
        *(buttonRegistry.registerButton(K11_SW, LED_K11, 112, DEBOUNCE_TIME)),
        *(buttonRegistry.registerButton(K12_SW, LED_K12, 113, DEBOUNCE_TIME)),
        *(buttonRegistry.registerButton(K13_SW, LED_K13, 114, DEBOUNCE_TIME)),
        *(buttonRegistry.registerButton(K14_SW, LED_K14, 115, DEBOUNCE_TIME)),
        *(buttonRegistry.registerButton(K15_SW, LED_K15, 116, DEBOUNCE_TIME)),
        *(buttonRegistry.registerButton(K16_SW, LED_K16, 117, DEBOUNCE_TIME))};

    Button trackButtons[NUM_TRACKS] = {
        *(buttonRegistry.registerButton(P1_SW, LED_P1, 20, DEBOUNCE_TIME)),
        *(buttonRegistry.registerButton(P2_SW, LED_P2, 21, DEBOUNCE_TIME)),
        *(buttonRegistry.registerButton(P3_SW, LED_P3, 22, DEBOUNCE_TIME)),
        *(buttonRegistry.registerButton(P4_SW, LED_P4, 23, DEBOUNCE_TIME)),
        *(buttonRegistry.registerButton(P5_SW, LED_P5, 24, DEBOUNCE_TIME)),
        *(buttonRegistry.registerButton(P6_SW, LED_P6, 25, DEBOUNCE_TIME)),
        *(buttonRegistry.registerButton(P7_SW, LED_P7, 26, DEBOUNCE_TIME)),
        *(buttonRegistry.registerButton(P8_SW, LED_P8, 27, DEBOUNCE_TIME))};

    Potentiometer trackKnobs[NUM_TRACKS] = {Potentiometer(KNOB_1, 14, true), Potentiometer(KNOB_2, 15, true), Potentiometer(KNOB_3, 28, true), Potentiometer(KNOB_4, 29, true), Potentiometer(KNOB_5, 30, true), Potentiometer(KNOB_6, 31, true), Potentiometer(KNOB_7, 118, true), Potentiometer(KNOB_8, 119, true)};

    Potentiometer trackSliders[NUM_TRACKS] = {
        Potentiometer(SLIDE_1, 85),
        Potentiometer(SLIDE_2, 86),
        Potentiometer(SLIDE_3, 87),
        Potentiometer(SLIDE_4, 88),
        Potentiometer(SLIDE_5, 89),
        Potentiometer(SLIDE_6, 90),
        Potentiometer(SLIDE_7, 3),
        Potentiometer(SLIDE_8, 9)};
};

#endif