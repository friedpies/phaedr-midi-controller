#ifndef fader_h
#define fader_h

#include <Arduino.h>
#include "mcuButton.h"

/*
 * MCU-06: Knob CC Reassignment Reference (for Plan 05 InputManager wiring)
 *
 * The Potentiometer class is correct as-is — only the CC numbers passed to
 * its constructor in InputManager need changing:
 *
 * KNOB_1 → CC 16  (was CC 14)
 * KNOB_2 → CC 17  (was CC 15)
 * KNOB_3 → CC 18  (was CC 28)
 * KNOB_4 → CC 19  (was CC 29)
 * KNOB_5 → CC 20  (was CC 30)
 * KNOB_6 → CC 21  (was CC 31)
 * KNOB_7 → CC 22  (was CC 118)
 * KNOB_8 → CC 23  (was CC 119)
 *
 * Note on VPot format: Hardware uses physical analog potentiometers (not rotary
 * encoders), so sending absolute 0–127 values on CC 16–23 is correct. Logic Pro
 * interprets these as absolute VPot positions. True MCU VPot encoders use relative
 * format (0x41=CW+1, 0x01=CCW+1), which does not apply here.
 */

enum PickupState { SYNCED, OUT_OF_SYNC };

class Fader
{
public:
    // Default constructor — sentinel state; call setup() before use
    Fader();
    // Full constructor — midiChannel: 1-8 (fader 1 = channel 1, fader 8 = channel 8)
    Fader(int pin, int midiChannel);
    // setup() for two-phase init (default-construct array, then configure in init())
    void setup(int pin, int midiChannel);
    void read();

    void setDawValue(int value14bit);         // PICK-01: store DAW value, detect bank switch
    void enterPickupMode();                   // set OUT_OF_SYNC, reset lazy-reveal flag
    void setChannelButton(MCUButton* btn);   // wire to corresponding P1–P8 button

    // Noise threshold: proportionally equivalent to Potentiometer's ANALOG_NOISE=3
    // on 0-1023 ADC mapped to 0-16383 range. 3/1023 * 16383 ≈ 48.
    // Increase to 64 or 96 if Logic fader display shows jitter.
    static const int FADER_NOISE_THRESHOLD = 48;

    // PICK-02 / PICK-06: pickup deadband and bank switch threshold (all in 14-bit units)
    // PICKUP_DEADBAND: ~128 = ≈1 in 7-bit MIDI. Must be > FADER_NOISE_THRESHOLD (48).
    // Start at 128; increase if fader noise causes false pickups; decrease if feel is too loose.
    static const int PICKUP_DEADBAND        = 128;
    // BANK_SWITCH_THRESHOLD: DAW value delta that signals a bank switch vs fader automation.
    // 512 in 14-bit ≈ 4 in 7-bit MIDI. Tune up if automation causes false triggers.
    static const int BANK_SWITCH_THRESHOLD  = 512;

private:
    int _pin;
    int _midiChannel;
    int _lastFader14bit;

    int         _dawValue14bit  = -1;      // -1 = no DAW value received yet
    PickupState _pickupState    = SYNCED;
    bool        _blinkRevealed  = false;   // lazy reveal: true after first interaction in OUT_OF_SYNC
    MCUButton*  _channelBtn     = nullptr;
};

#endif
