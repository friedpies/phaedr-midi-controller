#include <Arduino.h>
#include <IoAbstraction.h>
#include "pinDefines.h"


void setup()
{
  // put your setup code here, to run once:
  switches.initialise(ioUsingArduino());                       // pull up logic is optional, defaults to PULL_DOWN buttons.
  switches.addSwitch(spinwheelClickPin, onClicked, NO_REPEAT); // NO_REPEAT is optional, sets the repeat interval in 100s of second.
}

void loop()
{
  // put your main code here, to run repeatedly:
  taskManager.runLoop();
}