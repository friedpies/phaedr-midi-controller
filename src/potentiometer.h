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

private:
    int _pin;
    int _ccNum;
    bool _invert;
    int lastReading = 0;
};

#endif