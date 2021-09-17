#include <Arduino.h>
#include <XPOModel.h>
#include <pinDefines.h>

XPOModel xpoModel;
void setup()
{
  // put your setup code here, to run once:
  Serial.println(K1_SW);
}

void loop()
{
  xpoModel.initialize();
  // put your main code here, to run repeatedly:
  // xpoModel.update();
}

// class Button(input pin)
// emitter --> on clicked
// readonly onClick