#ifndef led_budget_h
#define led_budget_h

#include <Arduino.h>

// Central LED power budget manager. Tracks each lit LED's intended brightness (0-255)
// and proportionally scales the actual PWM duty so the combined draw stays within the
// USB 500 mA budget (≈ 350 mA for LEDs after the Teensy baseline).
//
// TOTAL_PWM_BUDGET is the cap on the sum of PWM values across all currently lit LEDs.
// Each PWM unit (0-255) corresponds to ~20 mA / 255 ≈ 0.078 mA per LED, so 4400 units
// ≈ 345 mA with a small safety margin. When the sum of intended brightnesses exceeds
// this cap, every lit LED is scaled down by the same factor, preserving relative
// brightness (e.g. the ripple gradient) instead of hard-clipping individual LEDs.
class LedBudget
{
public:
    static const int TOTAL_PWM_BUDGET = 4400;
    static const int MAX_PIN          = 64;  // Teensy 3.5 uses pins 0-57

    // Sets a pin's intended brightness and recomputes scaling across all lit LEDs.
    // 0 turns the pin off and removes it from the lit set.
    static void set(int pin, uint8_t intended);

private:
    static uint8_t _intended[MAX_PIN];
    static int     _litPins[MAX_PIN];
    static int     _litCount;

    static void apply();
    static void addLit(int pin);
    static void removeLit(int pin);
};

#endif
