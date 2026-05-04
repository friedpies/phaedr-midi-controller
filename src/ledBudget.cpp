#include "ledBudget.h"
#include <SoftPWM.h>

uint8_t LedBudget::_intended[LedBudget::MAX_PIN] = {0};
int     LedBudget::_litPins[LedBudget::MAX_PIN]  = {0};
int     LedBudget::_litCount                     = 0;

void LedBudget::addLit(int pin)
{
    for (int i = 0; i < _litCount; i++) {
        if (_litPins[i] == pin) return;
    }
    if (_litCount < MAX_PIN) _litPins[_litCount++] = pin;
}

void LedBudget::removeLit(int pin)
{
    for (int i = 0; i < _litCount; i++) {
        if (_litPins[i] == pin) {
            _litPins[i] = _litPins[--_litCount];
            return;
        }
    }
}

void LedBudget::set(int pin, uint8_t intended)
{
    if (pin < 0 || pin >= MAX_PIN) return;
    _intended[pin] = intended;
    if (intended == 0) {
        removeLit(pin);
        SoftPWMSet(pin, 0);
        apply();
        return;
    }
    addLit(pin);
    apply();
}

void LedBudget::apply()
{
    int sum = 0;
    for (int i = 0; i < _litCount; i++) {
        sum += _intended[_litPins[i]];
    }
    if (sum == 0) return;

    if (sum <= TOTAL_PWM_BUDGET) {
        for (int i = 0; i < _litCount; i++) {
            int p = _litPins[i];
            SoftPWMSet(p, _intended[p]);
        }
    } else {
        for (int i = 0; i < _litCount; i++) {
            int p = _litPins[i];
            int scaled = (int)_intended[p] * TOTAL_PWM_BUDGET / sum;
            SoftPWMSet(p, (uint8_t)scaled);
        }
    }
}
