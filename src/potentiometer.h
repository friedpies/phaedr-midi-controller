#include "pinDefines.h"
#include <Arduino.h>

#ifndef potentiometer
#define potentiometer

class Potentiometer
{
public:
    Potentiometer(int pin, int ccNum, bool invert = false) : _pin(pin),
                                                             _ccNum(ccNum),
                                                             _invert(invert)
    {
    }
    void init();
    void read();
    static const int READ_RESOLUTION = 1024;
    static const int ANALOG_NOISE = 3; // fluctuation from reading

private:
    int _pin;
    int _ccNum;
    bool _invert;
    int lastReading = 0;
    bool hasChanged(int newValue);
};

#endif