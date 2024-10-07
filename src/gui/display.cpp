#include "display.h"

#include "config.h"
#include "graphic.h"

portMUX_TYPE Display::m_mutex = portMUX_INITIALIZER_UNLOCKED;

Display::Display() {}

bool Display::init() {
    if (!gfx.begin()) {
        Serial.println(F("Display init failed!"));
        return false;
    }
    gfx.setRotation(3);
    gfx.fillScreen(BACKGROUND_COLOR);

    gfx.setTextSize(1);
    gfx.setTextColor(WHITE);
    gfx.setTextWrap(false);
#if defined(USE_DEFAULT_FONT) || SIMPLE_GUI == 1
    gfx.setFont(NULL);
#else
    gfx.setFont(Fonts::getFont(1));
#endif
    return true;
}

void Display::lock() {
    taskENTER_CRITICAL(&Display::m_mutex);
}
void Display::unlock() {
    taskEXIT_CRITICAL(&Display::m_mutex);
}

void Display::welcome() {
#if SIMPLE_GUI == 1
    gfx.setCursor(0, 0);
    gfx.println(F("Setup start..."));
#else
    gfx.setFont(Fonts::getFont(3));
    gfx.setCursor(10, gfx.height() / 2 + 10);
    gfx.println(F("DPF indicator is starting"));
    if (testMode) {
        gfx.setCursor(90, gfx.height() / 2 + 30);
        gfx.println(F("(test mode)"));
    }
    gfx.setFont(Fonts::getFont(1));
#endif
}