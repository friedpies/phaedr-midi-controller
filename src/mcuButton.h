#ifndef mcu_button_h
#define mcu_button_h

#include <Arduino.h>
#include <Bounce2.h>
#include <SoftPWM.h>

class MCUButton
{
public:
    // Default constructor — sentinel state; call setup() before use
    MCUButton();
    // Full constructor — initializes immediately
    MCUButton(int buttonPin, int ledPin, int noteNum, int debounceTime);
    // setup() for two-phase init (default-construct array, then configure in init())
    void setup(int buttonPin, int ledPin, int noteNum, int debounceTime);
    void init();
    void read();
    bool poll();                         // debounce update only — returns true on press, no MIDI output
    void setLedState(uint8_t velocity);  // 0=off, 127=on; 1=blink handled Phase 2
    int getLedPin() const;
    int getNoteNum() const;

private:
    int _buttonPin;
    int _ledPin;
    int _noteNum;
    int _debounceTime;
    bool _hasLed;  // false for buttons with no LED (Stop btn 15, Play btn 16)
    Bounce _bounce;
};

#endif
