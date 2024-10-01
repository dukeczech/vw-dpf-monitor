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
