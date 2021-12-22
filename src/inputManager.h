#ifndef button_manager
#define button_manager

#include <Arduino.h>
#include "button.h"
#include "pinDefines.h"
#include "potentiometer.h"

class InputManager
{
public:
    void init();
    void readAll();
    static const int NUM_GRID_BUTTONS = 16;
    static const int NUM_TRACKS = 8;
    static const int DEBOUNCE_TIME = 50;

    Button gridButtons[NUM_GRID_BUTTONS] = {
        Button(K1_SW, LED_K1, DEBOUNCE_TIME),
        Button(K2_SW, LED_K2, DEBOUNCE_TIME),
        Button(K3_SW, LED_K3, DEBOUNCE_TIME),
        Button(K4_SW, LED_K4, DEBOUNCE_TIME),
        Button(K5_SW, LED_K5, DEBOUNCE_TIME),
        Button(K6_SW, LED_K6, DEBOUNCE_TIME),
        Button(K7_SW, LED_K7, DEBOUNCE_TIME),
        Button(K8_SW, LED_K8, DEBOUNCE_TIME),
        Button(K9_SW, LED_K9, DEBOUNCE_TIME),
        Button(K10_SW, LED_K10, DEBOUNCE_TIME),
        Button(K11_SW, LED_K11, DEBOUNCE_TIME),
        Button(K12_SW, LED_K12, DEBOUNCE_TIME),
        Button(K13_SW, LED_K13, DEBOUNCE_TIME),
        Button(K14_SW, LED_K14, DEBOUNCE_TIME),
        Button(K15_SW, LED_K15, DEBOUNCE_TIME),
        Button(K16_SW, LED_K16, DEBOUNCE_TIME)};

    Button trackButtons[NUM_TRACKS] = {
        Button(P1_SW, LED_P1, DEBOUNCE_TIME),
        Button(P2_SW, LED_P2, DEBOUNCE_TIME),
        Button(P3_SW, LED_P3, DEBOUNCE_TIME),
        Button(P4_SW, LED_P4, DEBOUNCE_TIME),
        Button(P5_SW, LED_P5, DEBOUNCE_TIME),
        Button(P6_SW, LED_P6, DEBOUNCE_TIME),
        Button(P7_SW, LED_P7, DEBOUNCE_TIME),
        Button(P8_SW, LED_P8, DEBOUNCE_TIME)};

    Potentiometer trackKnobs[NUM_TRACKS] = {
        Potentiometer(KNOB_1, 20),
        Potentiometer(KNOB_2, 21),
        Potentiometer(KNOB_3, 22),
        Potentiometer(KNOB_4, 23),
        Potentiometer(KNOB_5, 24),
        Potentiometer(KNOB_6, 25),
        Potentiometer(KNOB_7, 26),
        Potentiometer(KNOB_8, 26)};
};

#endif