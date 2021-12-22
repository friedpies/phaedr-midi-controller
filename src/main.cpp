#include <Arduino.h>
#include "pinDefines.h"
#include "buttonManager.h"
#include "button.h"

int kSwitches[] = {
    K1_SW,
    K2_SW,
    K3_SW,
    K4_SW,
    K5_SW,
    K6_SW,
    K7_SW,
    K8_SW,
    K9_SW,
    K10_SW,
    K11_SW,
    K12_SW,
    K13_SW,
    K14_SW,
    K15_SW,
    K16_SW // HOOKED UP TO USB NATIVE PORTS, DUMB
};

// Button gridButtons[16] = createButtons();

// Button *createButtons()
// {
//   // Button buttons[16] = {};
//   return buttons;
// }

int kLEDs[] = {
    LED_K1,
    LED_K2,
    LED_K3,
    LED_K4,
    LED_K5,
    LED_K6,
    LED_K7,
    LED_K8,
    LED_K9,
    LED_K10,
    LED_K11,
    LED_K12,
    LED_K13,
    LED_K14,
    LED_K15,
    LED_K16};

int pSwitches[] = {
    P1_SW,
    P2_SW,
    P3_SW,
    P4_SW,
    P5_SW,
    P6_SW,
    P7_SW,
    P8_SW};

int pLEDs[] = {
    LED_P1,
    LED_P2,
    LED_P3,
    LED_P4,
    LED_P5,
    LED_P6,
    LED_P7,
    LED_P8};

int knobs[] = {
    KNOB_1,
    KNOB_2,
    KNOB_3,
    KNOB_4,
    KNOB_5,
    KNOB_6,
    KNOB_7,
    KNOB_8};

int sliders[] = {
    SLIDE_1,
    SLIDE_2,
    SLIDE_3,
    SLIDE_4,
    SLIDE_5,
    SLIDE_6,
    SLIDE_7,
    SLIDE_8};

ButtonManager buttonManager = ButtonManager();

void setup()
{
  buttonManager.init();
  
}

void loop()
{
Serial.println("LOOP");
  buttonManager.readAll();
}

// class Button(input pin)
// emitter --> on clicked
// readonly onClick