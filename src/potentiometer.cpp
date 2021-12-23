#include "potentiometer.h"

void Potentiometer::init()
{
}

void Potentiometer::read()
{
    int value = analogRead(_pin);
    if (_invert)
    {
        value = READ_RESOLUTION - value;
    }
    if (hasChanged(value))
    {
        lastReading = value;
        int mapped = map(lastReading, 0, 1023, 0, 127);
        // if (_pin == SLIDE_1)
        // {
        //     Serial.println(lastReading);
        // }
        usbMIDI.sendControlChange(_ccNum, mapped, 1);
        // }
    }
}

bool Potentiometer::hasChanged(int newValue)
{
    return (newValue >= (lastReading + ANALOG_NOISE) || newValue <= (lastReading - ANALOG_NOISE));
}