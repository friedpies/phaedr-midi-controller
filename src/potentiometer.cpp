#include "potentiometer.h"

void Potentiometer::init()
{
}

void Potentiometer::read()
{
    int value = analogRead(_pin);
    if (_invert)
        value = READ_RESOLUTION - value;

    if (!hasChanged(value)) return;

    if (_relative) {
        if (lastReading < 0) {
            // First read: capture current position without sending — avoids
            // slamming the DAW parameter to an arbitrary position on power-on.
            lastReading = value;
            return;
        }
        _relAccum -= (value - lastReading);  // negate: physical CW → positive steps → CW MIDI
        lastReading = value;

        int steps = _relAccum / ADC_PER_STEP;
        _relAccum %= ADC_PER_STEP;  // carry remainder into next call
        if (steps == 0) return;

        // MCU VPot sign-magnitude relative format (Logic Pro / Mackie Control):
        //   CW:  0x01-0x3F (bit 6 = 0, bits 0-5 = speed 1-63)
        //   CCW: 0x41-0x7F (bit 6 = 1, bits 0-5 = speed 1-63)
        int absSteps = constrain(abs(steps), 1, 63);
        uint8_t msg = (steps > 0)
            ? (uint8_t)absSteps           // CW:  0x01-0x3F
            : (uint8_t)(absSteps | 0x40); // CCW: 0x41-0x7F
        usbMIDI.sendControlChange(_ccNum, msg, 1);
    } else {
        lastReading = value;
        usbMIDI.sendControlChange(_ccNum, map(lastReading, 0, 1023, 0, 127), 1);
    }
}

bool Potentiometer::hasChanged(int newValue)
{
    if (lastReading < 0) return true;  // always fire on first read
    return (newValue >= (lastReading + ANALOG_NOISE) || newValue <= (lastReading - ANALOG_NOISE));
}
