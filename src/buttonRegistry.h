#ifndef button_registry
#define button_registry

#include <Arduino.h>
#include <map>
#include "button.h"

class ButtonRegistry
{
public:
    Button *registerButton(int buttonPin, int ledPin, int ccNum, int debounceTime);
    std::map<int, Button*> ccNumToButton;
};

#endif