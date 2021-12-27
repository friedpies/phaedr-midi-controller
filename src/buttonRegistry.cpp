#include "buttonRegistry.h"

Button *ButtonRegistry::registerButton(int buttonPin, int ledPin, int ccNum, int debounceTime)
{
    Button *newButton = new Button(buttonPin, ledPin, ccNum, debounceTime);
    ccNumToButton[ccNum] = newButton;
    return newButton;
}