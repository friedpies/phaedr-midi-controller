#include "noteRegistry.h"

void NoteRegistry::registerButton(uint8_t noteNum, MCUButton* btn)
{
    noteToButton[noteNum] = btn;
}

void NoteRegistry::unregisterButton(uint8_t noteNum)
{
    noteToButton.erase(noteNum);
}

MCUButton* NoteRegistry::getButton(uint8_t noteNum) const
{
    auto it = noteToButton.find(noteNum);
    if (it != noteToButton.end()) {
        return it->second;
    }
    return nullptr;
}
