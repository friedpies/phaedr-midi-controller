#ifndef button_h
#define button_h

#include <Arduino.h>
#include <functional>
#include <Bounce2.h>
// #include <iostream>

class Button
{
public:
    Button(uint8_t buttonPin, uint8_t ledPin, uint32_t debounceTime) : _buttonPin(buttonPin),
                                                                       _ledPin(ledPin),
                                                                       _debounceTime(debounceTime)
    {
    }

    void init();
    void read();

private:
    uint8_t _buttonPin;
    uint8_t _ledPin;
    uint32_t _debounceTime;
    Bounce2::Button button = Bounce2::Button();
    // void _onPressed();
};

#endif