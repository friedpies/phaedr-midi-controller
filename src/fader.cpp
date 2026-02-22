#include "fader.h"

Fader::Fader()
    : _pin(-1), _midiChannel(1), _lastFader14bit(-1)
{
}

Fader::Fader(int pin, int midiChannel)
    : _pin(pin), _midiChannel(midiChannel), _lastFader14bit(-1)
{
}

void Fader::setup(int pin, int midiChannel)
{
    _pin = pin;
    _midiChannel = midiChannel;
    _lastFader14bit = -1;
}

void Fader::read()
{
    int raw = analogRead(_pin);  // 0-1023 (10-bit ADC on Teensy 3.5 default)

    // Map 10-bit ADC to 14-bit MCU fader range (0-16383)
    int fader14bit = map(raw, 0, 1023, 0, 16383);

    // Suppress output if change is within noise threshold
    if (_lastFader14bit < 0 || abs(fader14bit - _lastFader14bit) > FADER_NOISE_THRESHOLD) {
        _lastFader14bit = fader14bit;

        // CRITICAL: Teensyduino sendPitchBend expects signed -8192 to +8191.
        // MCU spec uses 0-16383. Apply -8192 offset to convert.
        // Without this offset, values above 8191 are clamped and Logic fader
        // only moves in the lower half of travel.
        int pitchBendValue = fader14bit - 8192;
        usbMIDI.sendPitchBend(pitchBendValue, _midiChannel);
    }
}
