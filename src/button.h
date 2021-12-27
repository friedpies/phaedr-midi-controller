#ifndef button_h
#define button_h

#include <Arduino.h>
#include <Bounce2.h>
#include <SoftPWM.h>

#include <MIDIUSB.h>

class Button
{
public:
    Button(int buttonPin, int ledPin, int ccNum, int debounceTime) : _buttonPin(buttonPin),
                                                                          _ledPin(ledPin),
                                                                          _ccNum(ccNum),
                                                                          _debounceTime(debounceTime)
    {
    }

    void init();
    void read();
    void setLedState(bool newState);
    int getLEDPin();

private:
    int _buttonPin;
    int _ledPin;
    int _ccNum;
    bool ledState = LOW;
    int ledStateToPWM(bool state);
    int _debounceTime;
    Bounce button = Bounce();
};

#endif