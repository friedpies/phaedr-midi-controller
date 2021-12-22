#include "button.h"

void Button::init()
{
    button.attach(_buttonPin, INPUT_PULLUP);
    button.interval(_debounceTime);
    pinMode(_ledPin, OUTPUT);
    digitalWrite(_ledPin, LOW);
}

void Button::read()
{
    button.update();
    if (button.pressed())
    {
        digitalWrite(_ledPin, HIGH);
        // std::cout << "Button: " << _buttonPin << "Pressed" << std::endl;
    }
}
