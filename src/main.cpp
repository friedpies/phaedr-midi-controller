#define ENABLEMIDI 1

#include <Arduino.h>

#ifdef ENABLEMIDI
#include <MIDIUSB.h>
#include <MIDI.h>
#include <MIDI.hpp>
#endif

#include "pinDefines.h"
#include "inputManager.h"
#include "button.h"

const int DEFAULT_MIDI_CHANNEL = 1;

InputManager inputManager = InputManager();
// void OnNoteOn(byte channel, byte note, byte velocity)
// {
//     digitalWrite(LED_K1, HIGH); // Any Note-On turns on LED
// }

// void OnNoteOFF(byte channel, byte note, byte velocity)
// {
//     digitalWrite(LED_K1, LOW); // Any Note-On turns on LED
// }
void handleControlChangeMessage(byte channel, byte ccNum, byte velocity)
{
    inputManager.handleControlChangeMessage(channel, ccNum, velocity);
}

void handleStart()
{
}

void handleClock()
{
}

void handleStop()
{
    // Serial.println("HANDLE STOP");
}

void setup()
{
    inputManager.init();
    usbMIDI.setHandleControlChange(handleControlChangeMessage);
    usbMIDI.setHandleStart(handleStart);
    usbMIDI.setHandleClock(handleClock);
    usbMIDI.setHandleStop(handleStop);
}

void loop()
{
    inputManager.readAll();
    usbMIDI.read();
}

// class Button(input pin)
// emitter --> on clicked
// readonly onClick