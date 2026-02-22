#include "mcuButton.h"
#include "pinDefines.h"

MCUButton::MCUButton(int buttonPin, int ledPin, int noteNum, int debounceTime)
    : _buttonPin(buttonPin), _ledPin(ledPin), _noteNum(noteNum),
      _debounceTime(debounceTime), _hasLed(ledPin >= 0)
{
}

void MCUButton::init()
{
    _bounce.attach(_buttonPin, INPUT_PULLUP);
    _bounce.interval(_debounceTime);
    if (_hasLed) {
        SoftPWMSet(_ledPin, 0);
        SoftPWMSetFadeTime(_ledPin, 125, 125);
    }
}

void MCUButton::read()
{
    _bounce.update();
    if (_bounce.fell()) {
        // MCU Note Bang: NoteOn immediately followed by NoteOff — no local LED toggle
        usbMIDI.sendNoteOn(_noteNum, 127, 1);   // channel 1, velocity 127 = pressed
        usbMIDI.sendNoteOff(_noteNum, 0, 1);    // channel 1, velocity 0 = released
        // DO NOT update LED here — Logic Pro sends LED state via NoteOn/Off callbacks
    }
}

void MCUButton::setLedState(uint8_t velocity)
{
    if (!_hasLed) return;
    if (velocity == 0) {
        SoftPWMSet(_ledPin, 0);                  // off
    } else if (velocity == 127) {
        SoftPWMSet(_ledPin, LED_MAX_BRIGHTNESS); // capped brightness — stays within USB 500mA budget
    }
    // velocity == 1 (blink) is handled in Phase 2 — ignore for now
}

int MCUButton::getLedPin() const { return _ledPin; }
int MCUButton::getNoteNum() const { return _noteNum; }
