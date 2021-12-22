#include "potentiometer.h"

void Potentiometer::init()
{
}

void Potentiometer::read()
{
    int value = analogRead(_pin);
    Serial.println(value);
}