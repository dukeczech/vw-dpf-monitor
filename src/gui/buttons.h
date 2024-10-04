#include <Arduino.h>

class Buttons {
   public:
    static uint8_t BUTTON_UP;
    static uint8_t BUTTON_DOWN;

    static void init();

    static bool isPressedUp();
    static bool isPressedDown();
};