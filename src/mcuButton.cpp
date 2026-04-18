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
        stopBlink();                     // stop blink, LED goes dark
    } else if (velocity == 127) {
        stopBlink();                     // stop blink first
        SoftPWMSet(_ledPin, LED_MAX_BRIGHTNESS);  // then set solid on
    }
    // velocity == 1 from Logic = "blink" command; handled by pickup FSM — ignore here
    // (pickup FSM manages its own startBlink/stopBlink lifecycle)
}

void MCUButton::startBlink(uint16_t periodMs)
{
    if (!_hasLed) return;
    _blinkPeriodMs = (periodMs < 100) ? 100 : periodMs;  // floor at 100ms
    if (!_blinking) {
        _blinking    = true;
        _blinkPhase  = true;  // start with LED on
        _lastBlinkMs = millis();
        SoftPWMSet(_ledPin, LED_MAX_BRIGHTNESS);
    }
    // If already blinking, only update period — do not reset phase to avoid flicker
}

void MCUButton::stopBlink()
{
    _blinking   = false;
    _blinkPhase = false;
    if (_hasLed) SoftPWMSet(_ledPin, 0);  // go dark immediately — no confirmation animation
}

void MCUButton::updateBlink()
{
    if (!_blinking || !_hasLed) return;
    uint32_t now        = millis();
    uint32_t halfPeriod = _blinkPeriodMs / 2;
    if (now - _lastBlinkMs >= halfPeriod) {
        _blinkPhase  = !_blinkPhase;
        SoftPWMSet(_ledPin, _blinkPhase ? LED_MAX_BRIGHTNESS : 0);
        _lastBlinkMs = now;
    }
}

bool MCUButton::isBlinking() const { return _blinking; }

int MCUButton::getLedPin() const { return _ledPin; }
int MCUButton::getNoteNum() const { return _noteNum; }
