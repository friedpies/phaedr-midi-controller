#include "inputManager.h"

void InputManager::init()
{
    for (int i = 0; i < NUM_GRID_BUTTONS; i++)
    {
        gridButtons[i].init();
    }
    for (int i = 0; i < NUM_TRACKS; i++)
    {
        trackButtons[i].init();
    }
}

void InputManager::handleControlChangeMessage(byte channel, byte ccNum, byte velocity)
{
    if (channel == 1) // TODO don't know how to do this in Cpp, likely a lookup table or something
    {
        if (ccNum >= 102 && ccNum <= 117)
        {
            int buttonIndex = ccNum - 102;
            Button button = gridButtons[buttonIndex];
            button.setLedState(velocity == 127 ? HIGH : LOW);
        }
        else if (ccNum >= 20 && ccNum <= 27)
        {
            int buttonIndex = ccNum - 27;
            Button button = trackButtons[buttonIndex];
            button.setLedState(velocity == 127 ? HIGH : LOW);
        }
    }
}

void InputManager::readAll()
{
    for (int i = 0; i < NUM_GRID_BUTTONS; i++)
    {
        gridButtons[i].read();
    }
    for (int i = 0; i < NUM_TRACKS; i++)
    {
        trackButtons[i].read();
        trackKnobs[i].read();
        trackSliders[i].read();
    }
}