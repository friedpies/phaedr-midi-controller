#include "mcuButton.h"
#include "pinDefines.h"

MCUButton::MCUButton()
    : _buttonPin(-1), _ledPin(-1), _noteNum(-1), _debounceTime(20), _hasLed(false)
{
}

MCUButton::MCUButton(int buttonPin, int ledPin, int noteNum, int debounceTime)
    : _buttonPin(buttonPin), _ledPin(ledPin), _noteNum(noteNum),
      _debounceTime(debounceTime), _hasLed(ledPin >= 0)
{
}

void MCUButton::setup(int buttonPin, int ledPin, int noteNum, int debounceTime)
{
    _buttonPin = buttonPin;
    _ledPin = ledPin;
    _noteNum = noteNum;
    _debounceTime = debounceTime;
    _hasLed = (ledPin >= 0);
}

void MCUButton::init()
{
    // Skip if not configured (sentinel state — buttonPin < 0)
    if (_buttonPin < 0) return;
    _bounce.attach(_buttonPin, INPUT_PULLUP);
    _bounce.interval(_debounceTime);
    if (_hasLed) {
        SoftPWMSet(_ledPin, 0);
        SoftPWMSetFadeTime(_ledPin, 125, 125);
    }
}

void MCUButton::read()
{
    // Skip entirely if not configured (default-constructed sentinel state)
    if (_buttonPin < 0) return;
    _bounce.update();
    // Skip MIDI output for unmapped buttons (noteNum < 0 = no assigned MCU note)
    if (_noteNum < 0) return;
    if (_bounce.fell()) {
        // MCU Note Bang: NoteOn immediately followed by NoteOff — no local LED toggle
        usbMIDI.sendNoteOn((byte)_noteNum, 127, 1);   // channel 1, velocity 127 = pressed
        usbMIDI.sendNoteOff((byte)_noteNum, 0, 1);    // channel 1, velocity 0 = released
        // DO NOT update LED here — Logic Pro sends LED state via NoteOn/Off callbacks
    }
}

bool MCUButton::poll()
{
    if (_buttonPin < 0) return false;
    _bounce.update();
    return _bounce.fell();
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
