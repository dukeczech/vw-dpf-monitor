#include "buttons.h"

uint8_t Buttons::BUTTON_UP = 0;
uint8_t Buttons::BUTTON_DOWN = 14;

void Buttons::init() {
    pinMode(BUTTON_UP, INPUT_PULLUP);
    pinMode(BUTTON_DOWN, INPUT_PULLUP);
}

bool Buttons::isPressedUp() {
    return digitalRead(BUTTON_UP) == 0;
}

bool Buttons::isPressedDown() {
    return digitalRead(BUTTON_DOWN) == 0;
}