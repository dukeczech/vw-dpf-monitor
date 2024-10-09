#include "display.h"

#include "config.h"
#include "graphic.h"
#include "gui/bitmaps/distance_icon.h"
#include "gui/bitmaps/dpf_icon.h"
#include "gui/bitmaps/soot_icon.h"
#include "gui/bitmaps/time_icon.h"
#include "gui/bitmaps/time_regeneration_icon.h"
#include "gui/buttons.h"
#include "measurement.h"

// First line
Cell cc1(gfx);
Cell cc2(gfx);
Cell cc3a(gfx);
Cell cc3b(gfx);

// Second line
Cell cc4(gfx);
Cell cc5(gfx);
Cell cc6(gfx);

DPFIcon dpfIcon(gfx);
BTIcon btIcon(gfx);
CommIcon commIcon(gfx);
WifiIcon wifiIcon(gfx);
FireIcon fireIcon(gfx);
StatusBar sb(gfx, 40);
ProgressBar pb(gfx, 15);

bool Display::m_dirty = false;
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

void Display::setDirty() {
    m_dirty = true;
}

void Display::setup() {
    // Init the GUI
    Display::lock();

    sb.setText("DPF indicator", 150);

    btIcon.setPosition(5, 3);
    commIcon.setPosition(30, 7);
    wifiIcon.setPosition(67, 5);
    fireIcon.setPosition(105, 3);
    dpfIcon.getValue().setPadding(-5, 0);

    // First line
    uint16_t ycelltop = sb.getHeight() - sb.getBorderHeight() + 6;

    cc1.setSize(gfx.width() / 3 + 1, 72).setPosition(0, ycelltop).setBorder(0);
    cc1.setLabel(&dpfIcon).getLabel()->setPadding(0, 0);

    cc2.setSize(gfx.width() / 3, 72).setPosition(cc1.getX() + cc1.getWidth() - 6, ycelltop).setBorder(0);
    cc2.setLabel(new ImageLabel(soot_icon)).getLabel()->setPadding(0, 0).setColor(0xdedb);
    cc2.setValue(new TextLabel("-.--", "g", 4)).getValue()->setPadding(-5, 0).setColor(0xdedb);

    cc3a.setSize(gfx.width() / 3 + 5, 75).setPosition(cc2.getX() + cc2.getWidth() - 5, ycelltop).setBorder(0);
    cc3a.setLabel(new ImageLabel(distance_icon)).getLabel()->setPadding(0, 0).setColor(0xafe6);
    cc3a.setValue(new TextLabel("-.-", "km", 4)).getValue()->setPadding(-5, 0).setColor(0xafe6);

    cc3b.setSize(gfx.width() / 3 + 5, 75).setPosition(cc2.getX() + cc2.getWidth() - 5, ycelltop).setBorder(0);
    cc3b.setLabel(new ImageLabel(time_regeneration_icon)).getLabel()->setPadding(0, 0).setColor(0xf800);
    cc3b.setValue(new TextLabel("-.-", "min", 4)).getValue()->setPadding(-5, 0).setColor(0xf800);
#if 1
    // Second line
    ycelltop += sb.getHeight() - sb.getBorderHeight() + cc1.getHeight() - 35;

    cc4.setSize(cc1.getWidth(), 25).setPosition(0, ycelltop).setBorder(0);

    cc5.setSize(cc2.getWidth(), 25).setPosition(cc2.getX(), ycelltop).setBorder(0);
    cc5.setValue(new TextLabel("(+0.00", "g)", 2)).getValue()->setPadding(-5, 0).setColor(0xdedb);

    cc6.setSize(cc3a.getWidth(), 25).setPosition(cc3a.getX(), ycelltop).setBorder(0);
    cc6.setValue(new TextLabel("(+0.0", "km)", 2)).getValue()->setPadding(-5, 0).setColor(0xafe6);
#endif

    btIcon.disable().display();
    commIcon.disable().display();
    wifiIcon.disable().display();
    Display::unlock();
}

void Display::clear() {
    Display::lock();
    gfx.fillRect(0, 0 + sb.getHeight(), gfx.width(), gfx.height() - sb.getHeight(), BACKGROUND_COLOR);
    Display::unlock();
}

void Display::display() {
    Display::lock();
    sb.display();

    if (state > Init) {
        // Clear the display only once after the setup
        static bool afterInit = false;
        if (!afterInit) {
            gfx.fillRect(0, 0 + sb.getHeight(), gfx.width(), gfx.height() - sb.getHeight(), BACKGROUND_COLOR);
            afterInit = true;
        }

        if (testMode) {
            const long rnd = random(0, 100);

            pb.setProgress((double)rnd).display(true);
            dpfIcon.setTemperature(((double)rnd) * 6.5);
        }

        // Cell #2 is sligthly wider that overpaints cell #1
        cc2.display(m_dirty);
        cc1.display(m_dirty);

        if (Regeneration::isRegenerating() || Buttons::isPressedDown()) {
            cc3b.display(true);
            cc6.disable();
        } else {
            cc3a.display(true);
            cc6.enable();
        }

        cc4.display(m_dirty);
        cc5.display(m_dirty);
        cc6.display(m_dirty);

        pb.display(true);

        m_dirty = false;
    }

    Display::unlock();
}

void Display::welcome() {
    Display::lock();
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
    Display::unlock();
}

void Display::regenerationEnd(const uint16_t topy, const double& time) {
    Display::lock();

    Cell lbl(gfx);
    lbl.setSize(gfx.width(), gfx.height() - topy).setPosition(0, topy);
    lbl.setLabel(new TextLabel("Regeneration end", 6)).getLabel()->setColor(0xafe6);
    lbl.setValue(new TextLabel(String(time, 1), " min", 6)).getValue()->setPadding(15, 0).setColor(0xafe6);
    lbl.enable().display(true);

    Display::unlock();
}
