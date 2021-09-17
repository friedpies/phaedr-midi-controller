#include "XPOModel.h"
#include <IoAbstraction.h>
#include <pinDefines.h>

XPOModel::XPOModel()
{
}

void XPOModel::_handleK1Pressed(uint8_t pin, bool heldDown)
{
    Serial.println(pin);
}

void XPOModel::initialize()
{
    switches.initialise(ioUsingArduino()); // pull up logic is optional, defaults to PULL_DOWN buttons.
    switches.addSwitch(K1_SW, XPOModel::_handleK1Pressed, NO_REPEAT); // NO_REPEAT is optional, sets the repeat interval in 100s of second.
    update();
}

void XPOModel::update()
{
    taskManager.runLoop();
}

// void KelpieIO::setKnobOnStartup(byte knobIndex)
// {
//     int knobValue = analogRead(_knobs[knobIndex].pinNum);
//     _knobs[knobIndex].setValue = knobValue;
//     _knobs[knobIndex].polledValue = knobValue;
// }

// // Detects if a knob was changed and returns the changed Potentiometer
// byte KelpieIO::getIndexOfChangedKnob(void)
// {
//     for (byte i = 0; i < 16; i++)
//     {
//         _knobs[i].polledValue = analogRead(_knobs[i].pinNum); // **the dimensions of this struct seem to be reversed**
//         if ((_knobs[i].polledValue - _knobs[i].setValue) > 3 || (_knobs[i].polledValue - _knobs[i].setValue) < -3) // if there is a significant change
//         {
//             _knobs[i].setValue = _knobs[i].polledValue; //update state
//             return i;
//             break;
//         }
//     }
//     return -1;
// }

// byte KelpieIO::getIndexOfChangedButton(void)
// {
//     byte changedIndex = -1;
//     for (byte i = 0; i < 4; i++)
//     {
//         if (_buttons[i].debouncedPin.update())
//         {
//             if (_buttons[i].debouncedPin.fallingEdge())
//             {
//                 _buttons[i].buttonState = !_buttons[i].buttonState;
//                 digitalWrite(_buttons[i].ledPinNum, _buttons[i].buttonState);
//                 return i;
//             }
//         }
//     }
//     return changedIndex;
// }

// Potentiometer KelpieIO::getKnob(byte knobIndex)
// {
//     return _knobs[knobIndex];
// }

// Button KelpieIO::getButton(byte buttonIndex)
// {
//     return _buttons[buttonIndex];
// }

// void KelpieIO::blinkMidiLED(bool value)
// {
//     if (value)
//     {
//         digitalWrite(MIDILED, HIGH);
//     }
//     else
//     {
//         digitalWrite(MIDILED, LOW);
//     }
// }

// void KelpieIO::bootupAnimation(void)
// {
//     byte delayTimeMS = 50;
//     byte numCycles = 5;
//     for (byte counter = 0; counter < numCycles; counter++)
//     {
//         digitalWrite(MIDILED, HIGH);
//         delay(delayTimeMS);
//         digitalWrite(MIDILED, LOW);
//         for (byte i = 0; i < 4; i++)
//         {
//             digitalWrite(_buttons[i].ledPinNum, HIGH);
//             delay(delayTimeMS);
//             digitalWrite(_buttons[i].ledPinNum, LOW);
//             delay(delayTimeMS);
//         }
//     }
// }