#include <Arduino.h>
#include <MIDIUSB.h>
#include <MIDI.h>
#include <MIDI.hpp>
#include "pinDefines.h"
#include "inputManager.h"
#include "button.h"

const int DEFAULT_MIDI_CHANNEL = 1;

InputManager inputManager = InputManager();

void setup()
{
    inputManager.init();
}

void loop()
{
    //    myButton.read();
    inputManager.readAll();
}

// class Button(input pin)
// emitter --> on clicked
// readonly onClick