#include "inputManager.h"

void InputManager::init()
{
    for (int i = 0; i < NUM_GRID_BUTTONS; i++)
    {
        gridButtons[i].init();
    }
    for (int i = 0; i < NUM_TRACKS; i++) {
        trackButtons[i].init();
    }
}

void InputManager::readAll()
{   
    for (int i = 0; i < NUM_GRID_BUTTONS; i++)
    {
        gridButtons[i].read();
    }
    for (int i = 0; i < NUM_TRACKS; i++) {
        trackButtons[i].read();
    }
}