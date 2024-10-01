#include <Arduino.h>

class Buttons {
   public:
    static uint8_t BUTTON_UP;
    static uint8_t BUTTON_DOWN;

    static void Init();

    static bool IsPressedUp();
    static bool IsPressedDown();
};