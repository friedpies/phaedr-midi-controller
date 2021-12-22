#include <Arduino.h>

#ifndef button_types
#define button_types

class LEDButton
{
public:
    LEDButton(uint8_t buttonPin, uint8_t ledPin) : buttonPin(buttonPin), ledPin(ledPin) {}
    uint8_t buttonPin;
    uint8_t ledPin;
};

#endif