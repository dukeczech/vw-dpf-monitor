#include "sound.h"

#include "config.h"

const uint8_t Sound::BUZZER_PIN = 2;

Sound::Sound() {}

void Sound::beep(const uint32_t freq, const uint32_t duration) {
    tone(BUZZER_PIN, freq, duration);
}

void Sound::beep1short() {
    tone(BUZZER_PIN, 3000, 500);
}

void Sound::beep1long() {
    tone(BUZZER_PIN, 3000, 1500);
}

void Sound::beep3long() {
    tone(BUZZER_PIN, 3500, 300);
    delay(300);
    noTone(BUZZER_PIN);
    delay(300);

    tone(BUZZER_PIN, 3500, 300);
    delay(300);
    noTone(BUZZER_PIN);
    delay(300);

    tone(BUZZER_PIN, 3500, 300);
    delay(300);
    noTone(BUZZER_PIN);
}

void Sound::beep3short() {
    tone(BUZZER_PIN, 3500, 200);
    delay(200);
    noTone(BUZZER_PIN);
    delay(200);

    tone(BUZZER_PIN, 3500, 200);
    delay(200);
    noTone(BUZZER_PIN);
    delay(200);

    tone(BUZZER_PIN, 3500, 200);
    delay(200);
    noTone(BUZZER_PIN);
}