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
            // digitalWrite(_ledPin, ledState);
            if (ledState == LOW)
            {
                usbMIDI.sendControlChange(_ccNum, 127, 1);
            }
            else
            {
                usbMIDI.sendControlChange(_ccNum, 0, 1);
            }
        }
        else
        {
            // Serial.println("ROSE");
        }
    }
    // digitalWrite(_ledPin, HIGH);
    // std::cout << "Button: " << _buttonPin << "Pressed" << std::endl;
}

void Button::setLedState(bool newState)
{
    ledState = newState; // TODO add underscore
    digitalWrite(_ledPin, ledState);
}

int Button::getLEDPin()
{
    return _ledPin;
}