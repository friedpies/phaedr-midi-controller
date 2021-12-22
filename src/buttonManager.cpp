#include "buttonManager.h"

ButtonManager::ButtonManager()
{
    for (int i = 0; i < NUM_GRID_BUTTONS; i++)
    {
        buttons[i] = Button(gridButtons->buttonPin, gridButtons->ledPin, 50);
        // buttons[i].begin();
    }
};

void ButtonManager::init()
{
    for (int i = 0; i < NUM_GRID_BUTTONS; i++)
    {
        buttons[i].init();
    }
}

void ButtonManager::readAll()
{   
    Serial.println("READALL");
    for (int i = 0; i < NUM_GRID_BUTTONS; i++)
    {
        buttons[i].read();
    }
}