#ifndef button_h
#define button_h

#include <Arduino.h>
#include <Bounce2.h>
#include <MIDIUSB.h>

class Button
{
public:
    Button(int buttonPin, int ledPin, int ccNum, uint32_t debounceTime) : _buttonPin(buttonPin),
                                                                          _ledPin(ledPin),
                                                                          _ccNum(ccNum),
                                                                          _debounceTime(debounceTime)
    {
    }

    void init();
    void read();

private:
    int _buttonPin;
    int _ledPin;
    int _ccNum;
    bool ledState = LOW;
    uint32_t _debounceTime;
    Bounce button = Bounce();
};

#endif