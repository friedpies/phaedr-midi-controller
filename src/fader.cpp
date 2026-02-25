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

void Fader::setChannelButton(MCUButton* btn) {
    _channelBtn = btn;
}

void Fader::enterPickupMode() {
    _pickupState   = OUT_OF_SYNC;
    _blinkRevealed = false;
    // NOTE: do NOT call _channelBtn->startBlink() here — blink is lazily revealed
    // on first user interaction, per CONTEXT.md: "no visual noise by default"
}

void Fader::setDawValue(int newValue14bit) {
    // PICK-05 / bank switch detection: if the DAW value changes by a large amount
    // while this fader is SYNCED, treat it as a bank switch and enter pickup mode.
    // Guard: _dawValue14bit >= 0 ensures we skip the check on first power-on receipt
    // (Pitfall 4 from RESEARCH.md — first bank switch after boot)
    if (_dawValue14bit >= 0 && _pickupState == SYNCED) {
        int delta = abs(newValue14bit - _dawValue14bit);
        if (delta > BANK_SWITCH_THRESHOLD) {
            enterPickupMode();
        }
    }
    _dawValue14bit = newValue14bit;
}

void Fader::read() {
    if (_pin < 0) return;

    int raw        = analogRead(_pin);  // 0-1023 (10-bit ADC on Teensy 3.5 default)

    // Map 10-bit ADC to 14-bit MCU fader range (0-16383)
    int fader14bit = map(raw, 0, 1023, 0, 16383);

    // Suppress output if change is within noise threshold
    // IMPORTANT: compute significantMove BEFORE early-return so _lastFader14bit is
    // only updated on meaningful changes, keeping the crossover check stable.
    bool significantMove = (_lastFader14bit < 0 ||
                            abs(fader14bit - _lastFader14bit) > FADER_NOISE_THRESHOLD);
    if (!significantMove) return;

    _lastFader14bit = fader14bit;

    if (_pickupState == OUT_OF_SYNC) {
        // --- Lazy blink reveal (CONTEXT.md: no visual noise on bank switch) ---
        // On first significant move while OUT_OF_SYNC: reveal the blink.
        // On subsequent moves: refresh blink period to encode current distance.
        if (_dawValue14bit >= 0 && _channelBtn != nullptr) {
            int distance = abs(fader14bit - _dawValue14bit);
            // Map distance 0..16383 → period 600..200ms (faster = farther from target)
            // Using Arduino map(): linear interpolation between the two extremes.
            uint16_t period = (uint16_t)map(
                min(distance, 16383), 0, 16383, 600, 200);
            _channelBtn->startBlink(period);
            _blinkRevealed = true;
        }

        // --- Crossover detection (PICK-02, PICK-04, PICK-06) ---
        // Boundary edge case (PICK-06): both physical and DAW at rail → immediate pickup
        bool bothAtZero = (fader14bit <= PICKUP_DEADBAND &&
                           _dawValue14bit >= 0 && _dawValue14bit <= PICKUP_DEADBAND);
        bool bothAtMax  = (fader14bit >= (16383 - PICKUP_DEADBAND) &&
                           _dawValue14bit >= (16383 - PICKUP_DEADBAND));
        // Normal crossover: physical enters deadband around DAW value
        bool inDeadband = (_dawValue14bit >= 0 &&
                           abs(fader14bit - _dawValue14bit) <= PICKUP_DEADBAND);

        if (bothAtZero || bothAtMax || inDeadband) {
            _pickupState = SYNCED;
            if (_channelBtn != nullptr) _channelBtn->stopBlink();
            // PICK-04: immediately send snap value so DAW snaps to match physical position
            // CRITICAL: Teensyduino sendPitchBend expects signed -8192 to +8191.
            // MCU spec uses 0-16383. Apply -8192 offset to convert.
            usbMIDI.sendPitchBend(fader14bit - 8192, _midiChannel);
        }
        // Suppress MIDI while OUT_OF_SYNC — return without sending
        return;
    }

    // SYNCED: normal pitch bend output
    // CRITICAL: Teensyduino sendPitchBend expects signed -8192 to +8191.
    // MCU spec uses 0-16383. Apply -8192 offset to convert.
    // Without this offset, values above 8191 are clamped and Logic fader
    // only moves in the lower half of travel.
    usbMIDI.sendPitchBend(fader14bit - 8192, _midiChannel);
}
