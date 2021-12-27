#include "button.h"

void Button::init()
{
    button.attach(_buttonPin, INPUT_PULLUP);
    button.interval(_debounceTime);
    SoftPWMSet(_ledPin, 0);
    SoftPWMSetFadeTime(_ledPin, 125, 125);
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
    SoftPWMSet(_ledPin, ledStateToPWM(ledState));
    // digitalWrite(_ledPin, ledState);
}

int Button::ledStateToPWM(bool state)
{
    return state ? 255 : 0;
}

int Button::getLEDPin()
{
    return _ledPin;
}