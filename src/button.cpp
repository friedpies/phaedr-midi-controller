#include "button.h"

void Button::init()
{
    button.attach(_buttonPin, INPUT_PULLUP);
    button.interval(_debounceTime);
    pinMode(_ledPin, OUTPUT);
    digitalWrite(_ledPin, ledState);
}

void Button::read()
{
    button.update();
    if (button.changed())
    {
        if (button.fell())
        {
            ledState = !ledState;
            digitalWrite(_ledPin, ledState);
        }
        else
        {
            // Serial.println("ROSE");
        }
    }
    // digitalWrite(_ledPin, HIGH);
    // std::cout << "Button: " << _buttonPin << "Pressed" << std::endl;
}
