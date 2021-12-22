#ifndef button_manager
#define button_manager

#include <Arduino.h>
#include "button.h"
#include "pinDefines.h"
#include "ledButton.h"

class ButtonManager
{
public:
    ButtonManager();
    void init();
    void readAll();
    static const int NUM_GRID_BUTTONS = 1;
    LEDButton gridButtons[NUM_GRID_BUTTONS] = {
        LEDButton(K1_SW, LED_K1)
    };
    // int gridButtonsPins[NUM_GRID_BUTTONS] = {
    //     K1_SW,
    //     K2_SW,
    //     K3_SW,
    //     K4_SW,
    //     K5_SW,
    //     K6_SW,
    //     K7_SW,
    //     K8_SW,
    //     K9_SW,
    //     K10_SW,
    //     K11_SW,
    //     K12_SW,
    //     K13_SW,
    //     K14_SW,
    //     K15_SW,
    //     K16_SW // HOOKED UP TO USB NATIVE PORTS, DUMB
    // };

    Button *buttons = (Button *)malloc(sizeof(Button) * NUM_GRID_BUTTONS);
};

#endif