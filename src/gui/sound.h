#include <Arduino.h>

class Sound {
   public:
    static const uint8_t BUZZER_PIN;

    Sound();

    static void beep(const uint32_t freq, const uint32_t duration);
    static void beep1short();
    static void beep1long();
    static void beep3long();
    static void beep3short();
};