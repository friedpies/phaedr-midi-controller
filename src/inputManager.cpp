#include "inputManager.h"

void InputManager::init()
{
    SoftPWMBegin();
    // SoftPWMSetFadeTime(ALL, 100, 100);
    // SoftPWMSetFadeTime(ALL, 500, 500);

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
    if (channel == 1)
    {
        if ((ccNum >= 102 && ccNum <= 117) || (ccNum >= 20 && ccNum <= 27))
        {
            Button button = *(buttonRegistry.ccNumToButton[ccNum]);
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