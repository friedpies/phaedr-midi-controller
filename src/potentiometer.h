#include "pinDefines.h"
#include <Arduino.h>

#ifndef potentiometer
#define potentiometer

class Potentiometer
{
public:
    Potentiometer(int pin, int ccNum, bool invert = false, bool relative = false)
        : _pin(pin), _ccNum(ccNum), _invert(invert), _relative(relative) {}
    void init();
    void read();
    static const int READ_RESOLUTION = 1024;
    static const int ANALOG_NOISE = 3;

    // ADC units accumulated per relative MIDI step — tuning knob for VPot sensitivity.
    // Full sweep = 1023 ADC / ADC_PER_STEP total steps sent to Logic.
    // 8 → ~128 steps; 16 → ~64 steps. Increase to slow down, decrease to speed up.
    static const int ADC_PER_STEP = 8;   // 1023 ADC / 8 ≈ 128 steps = full ±64 pan range

private:
    int _pin;
    int _ccNum;
    bool _invert;
    bool _relative;
    int lastReading = -1;  // -1 = not yet initialized (skips first-read send)
    int _relAccum   = 0;   // accumulated ADC delta not yet emitted as relative steps
    bool hasChanged(int newValue);
};

#endif
