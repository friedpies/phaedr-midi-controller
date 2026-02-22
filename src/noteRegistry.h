#ifndef note_registry_h
#define note_registry_h

#include <Arduino.h>
#include <map>
#include "mcuButton.h"

/*
 * MCU Note Number Reference (for Plan 05 wiring)
 *
 * Channel strip (MIDI ch 1):
 *   REC:    Ch1=0,  Ch2=1,  Ch3=2,  Ch4=3,  Ch5=4,  Ch6=5,  Ch7=6,  Ch8=7
 *   SOLO:   Ch1=8,  Ch2=9,  Ch3=10, Ch4=11, Ch5=12, Ch6=13, Ch7=14, Ch8=15
 *   MUTE:   Ch1=16, Ch2=17, Ch3=18, Ch4=19, Ch5=20, Ch6=21, Ch7=22, Ch8=23
 *   SELECT: Ch1=24, Ch2=25, Ch3=26, Ch4=27, Ch5=28, Ch6=29, Ch7=30, Ch8=31
 *
 * Transport (MIDI ch 1):
 *   Rewind=91, FastForward=92, Stop=93, Play=94, Record=95
 */

class NoteRegistry
{
public:
    void registerButton(uint8_t noteNum, MCUButton* btn);
    MCUButton* getButton(uint8_t noteNum) const;

    std::map<uint8_t, MCUButton*> noteToButton;
};

#endif
