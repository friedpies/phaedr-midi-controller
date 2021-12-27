#include "buttonRegistry.h"

Button *ButtonRegistry::registerButton(int buttonPin, int ledPin, int ccNum, uint32_t debounceTime)
{
    Button *newButton = new Button(buttonPin, ledPin, ccNum, debounceTime);
    ccNumToButton[ccNum] = newButton;
    return newButton;
}