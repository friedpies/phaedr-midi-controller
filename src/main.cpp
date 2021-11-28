#include <Arduino.h>
#include <XPOModel.h>
#include <pinDefines.h>

XPOModel xpoModel;

int kSwitches[] = {
    K1_SW,
    K2_SW,
    K3_SW,
    K4_SW,
    K5_SW,
    K6_SW,
    K7_SW,
    K8_SW,
    K9_SW,
    K10_SW,
    K11_SW,
    K12_SW,
    K13_SW,
    K14_SW,
    K16_SW // HOOKED UP TO USB NATIVE PORTS, DUMB
};

int kLEDs[] = {
    LED_K1,
    LED_K2,
    LED_K3,
    LED_K4,
    LED_K5,
    LED_K6,
    LED_K7,
    LED_K8,
    LED_K9,
    LED_K10,
    LED_K11,
    LED_K12,
    LED_K13,
    LED_K14,
    //LED_K15,
    LED_K16};

int pSwitches[] = {
    P1_SW,
    P2_SW,
    P3_SW,
    P4_SW,
    P5_SW,
    P6_SW,
    P7_SW,
    P8_SW};

int pLEDs[] = {
    LED_P1,
    LED_P2,
    LED_P3,
    LED_P4,
    LED_P5,
    LED_P6,
    LED_P7,
    LED_P8};

int knobs[] = {
    KNOB_1,
    KNOB_2,
    KNOB_3,
    KNOB_4,
    KNOB_5,
    KNOB_6,
    KNOB_7,
    KNOB_8};

int sliders[] = {
    SLIDE_1,
    SLIDE_2,
    SLIDE_3,
    SLIDE_4,
    SLIDE_5,
    SLIDE_6,
    SLIDE_7,
    SLIDE_8};

void setup()
{
  // put your setup code here, to run once:
  for (int i = 0; i < 14; i++)
  {
    pinMode(kSwitches[i], INPUT_PULLUP);
  }

  for (int i = 0; i < 14; i++)
  {
    pinMode(kLEDs[i], OUTPUT);
    digitalWrite(kLEDs[i], LOW);
  }

  for (int i = 0; i < 8; i++)
  {
    pinMode(pSwitches[i], INPUT_PULLUP);
    pinMode(pLEDs[i], OUTPUT);
    digitalWrite(pLEDs[i], LOW);
  }
}

void loop()
{
  for (int i = 0; i < 14; i++)
  {
    int value = digitalRead(kSwitches[i]);
    digitalWrite(kLEDs[i], !value);
    Serial.print("G");
    Serial.print(":");
    Serial.print(value);
    Serial.print("  ");
  }

  for (int i = 0; i < 8; i++)
  {
    int value = digitalRead(pSwitches[i]);
    digitalWrite(pLEDs[i], !value);
    Serial.print("P");
    Serial.print(":");
    Serial.print(value);
    Serial.print("  ");
  }

  for (int i = 0; i < 8; i++)
  {
    int value = analogRead(knobs[i]);
    Serial.print("K");
    Serial.print(":");
    Serial.print(value);
    Serial.print("  ");
  }

  for (int i = 0; i < 8; i++)
  {
    int value = analogRead(sliders[i]);
    Serial.print("S");
    Serial.print(":");
    Serial.print(value);
    Serial.print("  ");
    if (i == 7)
    {
      Serial.println();
    }
  }
}

// class Button(input pin)
// emitter --> on clicked
// readonly onClick