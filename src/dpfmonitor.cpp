#include <Arduino.h>
#include <float.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <map>

#include "Every.h"
#include "config.h"
#include "gui/bitmaps/distance_icon.h"
#include "gui/bitmaps/dpf_icon.h"
#include "gui/bitmaps/soot_icon.h"
#include "gui/bitmaps/time_icon.h"
#include "gui/bitmaps/time_regeneration_icon.h"
#include "gui/buttons.h"
#include "gui/display.h"
#include "gui/graphic.h"
#include "gui/sound.h"
#include "measurement.h"
#include "obd.h"
#include "storage.h"

// Interesting page https://github.com/blizniukp/WIFI_kit_32_dpf/tree/main
// https://github.com/jazdw/vag-blocks
// 067,3,Pressure Differential,DPF,Specification (Idle/Clean DPF): 3.0...7.0 mbar\nSpecification (Partitally clean DPF): 30.0...70.0 mbar\nSpecification (Fully Loaded DPF): max. 300.0 mbar

Stream* channel = NULL;
Every measureAction(5000);
Every btAction(1000);
Every::Toggle statusAction(1000);

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
FireIcon fireIcon(gfx);
StatusBar sb(gfx, 40);
ProgressBar pb(gfx, 15);

void initGUI();
void displayGUI();
void idle();

class IconStatusCallback : public BLEStatusCallback {
   public:
    IconStatusCallback() {
        m_connected = false;
    }

    virtual ~IconStatusCallback() override {}

    bool IsConnected() { return m_connected; }

    virtual void onConnected() override {
        m_connected = true;

        Display::lock();
        btIcon.enable().display();
        Display::unlock();
    }

    virtual void onDisconnected() override {
        m_connected = false;

        Display::lock();
        btIcon.disable().display();

        // Turn off the receice status
        commIcon.disable().display();
        Display::unlock();
    }

    virtual void onTransmit() override {
        Display::lock();
        commIcon.enable().display();
        Display::unlock();
    }

    virtual void onReceive() override {
        Display::lock();
        commIcon.disable().display();
        Display::unlock();
    }

   protected:
    bool m_connected;
    bool m_channelactive;
};

IconStatusCallback callback;

void initGUI() {
    // Init the GUI
    Display::lock();

    sb.setText("DPF indicator", 120);

    btIcon.setPosition(5, 3);
    commIcon.setPosition(30, 7);
    fireIcon.setPosition(70, 3);
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
    Display::unlock();
}

void displayGUI() {
    const long rnd = random(0, 100);

    Display::lock();
    if (testMode) {
        pb.setProgress((double)rnd).display(true);
        dpfIcon.setTemperature(((double)rnd) * 6.5);
    }

    sb.display();
    cc1.display();
    cc2.display();

    if (Regeneration::isRegenerating() || Buttons::isPressedDown()) {
        cc3b.display(true);
        cc6.disable();
    } else {
        cc3a.display(true);
        cc6.enable();
    }

    cc4.display();
    cc5.display();
    cc6.display();

    // dpfIcon.display();
    pb.display(true);
    Display::unlock();
}

bool connect() {
#if BUILD_ENV_NAME == lilygo_t_display_s3
    // Try the connection to be opened
    if (serialBT.IsConnected()) return true;

    (testMode) ? serialBT.Scan(2) : serialBT.Scan();

    serialBT.Loop();

    channel = &serialBT;

    if (serialBT.IsConnected()) {
        Serial.println(F("Bluetooth channel is opened"));
#endif
        if (!myELM327.begin(*channel, false, 2000)) {
            Serial.println(F("ELM327 begin failed"));

#if BUILD_ENV_NAME == lolin32_lite
            display.setCursor(0, 10);
            display.println(F("ELM327 begin failed"));
            display.display();

            while (true) {
            };
#endif
            return false;
        } else {
            Serial.println(F("ELM327 is connected"));
        }

        OBD::init();
        Serial.println(F("OBD init OK"));

        return true;
#if BUILD_ENV_NAME == lilygo_t_display_s3
    }
#endif
    return false;
}

void setup() {
    Serial.begin(115200);

    // Will wait for up to ~2 second for Serial to connect.
    delay(2000);

    Serial.println(F("DPF indicator setup start"));

    // Init the buttons
    Buttons::init();

    if (Buttons::isPressedDown()) {
        testMode = true;

        // Simulate the measure every second
        measureAction = Every(1000);
        Serial.println(F("Test mode is enabled"));
    }

    // Storage init
    Storage::init();

    // Init measurements
    Measurements::init();

#if BUILD_ENV_NAME == lolin32_lite
    Wire.setPins(I2C_SDA, I2C_SCL);

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        return;
    }

    display.setTextSize(1);  // Normal 1:1 pixel scale
    // display.setTextColor(SSD1306_WHITE);  // Draw white text
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);  // Draw white text
    display.setTextWrap(false);
    display.setCursor(0, 0);  // Start at top-left corner
    Serial.println(F("SSD1306 allocation OK"));

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("Setup start..."));
    display.display();

#if RANDOM_DATA == 1
    display.setCursor(0, 10);
    display.println(F("Setup end..."));
    display.display();

    display.clearDisplay();
    return;
#endif

    serialBT.begin("dpfread", true);
    serialBT.setPin("1234");

    /*Serial.println("Starting synchronous discovery... ");
    BTScanResults *pResults = serialBT.discover(10000);
    if (pResults) {
      pResults->dump(&Serial);
    } else {
      Serial.println("Error on BT Scan, no result!");
    }*/

    if (!serialBT.connect("V-LINK")) {
        Serial.println(F("OBD connect KO [1]"));

        display.setCursor(0, 10);
        display.println(F("OBD connect KO [1]"));
        display.display();

        // If failed try ELM327 MAC address 10:21:3E:4D:2A:17
        uint8_t address[6] = {0x10, 0x21, 0x3E, 0x4D, 0x2A, 0x17};
        if (!serialBT.connect(address)) {
            Serial.println(F("OBD connect KO [2]"));

            display.setCursor(0, 10);
            display.println(F("OBD connect KO [2]"));
            display.display();
        }

        while (true) {
        };
    }
    channel = &serialBT;
#endif

#if BUILD_ENV_NAME == lilygo_t_display_s3
    // Init the display
    GFX_EXTRA_PRE_INIT();

#ifdef GFX_BL
    pinMode(GFX_BL, OUTPUT);
    digitalWrite(GFX_BL, HIGH);
#endif

    Display::init();

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
    initGUI();
    idle();

    serialBT.SetCommunicationCallback(&callback);
    serialBT.Init();

    Serial.println(F("Bluetooth init end"));
#endif

    connect();

#if BUILD_ENV_NAME == lolin32_lite
    display.setCursor(0, 10);
    display.println(F("Setup end..."));
    display.display();

    display.clearDisplay();
#endif

#if BUILD_ENV_NAME == lilygo_t_display_s3
#if SIMPLE_GUI == 1
    gfx.setCursor(0, 16);
    gfx.println(F("Setup end..."));
#endif
    // Call fill screen twice only works (wtf??)
    gfx.fillRect(0, 0 + sb.getHeight(), gfx.width(), gfx.height() - sb.getHeight(), BACKGROUND_COLOR);

    displayGUI();
#endif
    Serial.println(F("DPF indicator setup end"));
}

void displayLine(uint8_t line, const char* label, const double value, const uint8_t precision, const char* unit) {
#if BUILD_ENV_NAME == lolin32_lite
    display.setCursor(0, line * 8);
    display.print("                           ");
    display.setCursor(0, line * 8);
    display.printf("%s: %.*f %s\n", label, precision, value, unit);
    display.display();
#endif
#if BUILD_ENV_NAME == lilygo_t_display_s3
    gfx.fillRect(0, line * 16, gfx.width() - 10, 16, BLACK);
    gfx.setCursor(0, line * 16);
    gfx.printf("%s: %.*f %s\n", label, precision, value, unit);
#endif
}

void idle() {
    Display::lock();
    if (statusAction()) {
        sb.display();

        if (Regeneration::check() || Buttons::isPressedDown())
            if (statusAction.state)
                fireIcon.enable().display();
            else
                fireIcon.disable().display();
        else
            fireIcon.disable().display();
    }
    Display::unlock();
}

void loop() {
#if BUILD_ENV_NAME == lilygo_t_display_s3
    // Check the bluetooth status every 1s
    if (btAction()) {
        if (!serialBT.IsConnected() && !testMode) {
            // Reconnect
            if (connect()) {
                displayGUI();
            }
        }
    }
#endif

    // Process idle
    idle();

    // Iterate over measurements map every 5s
    bool measureOK = true;
    if (measureAction()) {
        for (auto itr = Measurements::getActual().begin(); itr != Measurements::getActual().end(); ++itr) {
            if (itr->second.enabled) {
#if BUILD_ENV_NAME == lilygo_t_display_s3
                // Check the connection status
                if (!serialBT.IsConnected() && !testMode) {
                    Serial.println(F("Bluetooth channel disconnected"));
                    measureOK = false;
                    break;
                }
#endif

                const bool result = Measurements::measure(itr->second, testMode);
                if (!result) {
                    // Measurement error
                    Serial.printf("Measurement error: %s\n", itr->second.caption);
                    measureOK = false;
                    break;
                }

                // Recalculate soot load
                if (itr->first == SOOT_MASS_CALCULATED) {
                    Measurements::getValue(SOOT_LOAD) = min(Measurements::getValue(SOOT_MASS_CALCULATED) / 0.24, 100.0);
                }
            }
        }

        // Check the first run and assign the measurements at beginning
        if (measureOK && Measurements::getStart().empty()) {
            Measurements::copy();
        }

        // Simulate the regeneration start
        if (Buttons::isPressedUp()) {
            if (Regeneration::isRegenerating()) {
                Measurements::setValue(REGENERATION_DURATION, 0.0);
                Measurements::setValue(DPF_INPUT_TEMPERATURE, 250.0);
                Measurements::setValue(POST_INJECTION_2, 0.0);
                Measurements::setValue(POST_INJECTION_3, 0.0);
            } else {
                Measurements::setValue(REGENERATION_DURATION, 0.1);
                Measurements::setValue(DPF_INPUT_TEMPERATURE, 400.0);
            }
        }

        //  Display the measurement data
        uint8_t line = 0;
        Serial.println("------------------------------------------");
        for (auto itr = Measurements::getActual().begin(); itr != Measurements::getActual().end(); ++itr) {
            if (itr->second.enabled) {
                Serial.printf("%s: %.*f %s\n", itr->second.caption, itr->second.precision, itr->second.value, itr->second.unit);
#if SIMPLE_GUI == 1
                displayLine(line, itr->second.shortCaption, itr->second.value, itr->second.precision, itr->second.unit);
#endif
                line++;
#if BUILD_ENV_NAME == lolin32_lite
                if (line >= 7) {
                    line = 0;
                }
#endif
#if BUILD_ENV_NAME == lilygo_t_display_s3
                if (line >= 10) {
                    line = 0;
                }
#endif
            }
        }
        if (testMode) {
            Serial.print(Measurements::toString());
        }
        Serial.println("------------------------------------------");

        // Check the regeneration process
        if (Regeneration::check()) {
            // TODO
        }

        // Update the GUI values
        if (Measurements::isEnabled(DPF_INPUT_TEMPERATURE)) {
            dpfIcon.setTemperature(Measurements::getValue(DPF_INPUT_TEMPERATURE));
        }
        if (Measurements::isEnabled(SOOT_MASS_CALCULATED)) {
            if (cc2.getValue() != NULL) ((TextLabel*)cc2.getValue())->setText(String(Measurements::getValue(SOOT_MASS_CALCULATED), 2));

            const double val = Measurements::diff(SOOT_MASS_CALCULATED);
            if (val >= 0.0) {
                if (cc5.getValue() != NULL) ((TextLabel*)cc5.getValue())->setText("(+" + String(val, 2));
            } else {
                if (cc5.getValue() != NULL) ((TextLabel*)cc5.getValue())->setText("(" + String(val, 2));
            }
        }
        if (Measurements::isEnabled(DISTANCE_SINCE_LAST_REGENERATION)) {
            if (cc3a.getValue() != NULL) ((TextLabel*)cc3a.getValue())->setText(String(Measurements::getValue(DISTANCE_SINCE_LAST_REGENERATION), 1));

            const double val = Measurements::diff(DISTANCE_SINCE_LAST_REGENERATION);
            if (val >= 0.0) {
                if (cc6.getValue() != NULL) ((TextLabel*)cc6.getValue())->setText("(+" + String(val, 1));
            } else {
                if (cc6.getValue() != NULL) ((TextLabel*)cc6.getValue())->setText("(" + String(val, 1));
            }
        }
        if (Measurements::isEnabled(REGENERATION_DURATION)) {
            if (cc3b.getValue() != NULL) ((TextLabel*)cc3b.getValue())->setText(String(Measurements::getValue(REGENERATION_DURATION), 1));
        }

        // Soot load is always disabled
        pb.setProgress(Measurements::getValue(SOOT_LOAD));

        // Update the GUI
        displayGUI();
    }

    // How to check the regeneration start?
    // Mass calculated > 24g
    // Mass calculated decreases
    // Post injection is > 0
    // Input temp > 380C

    // How to check the regeneration end?
    // Regeneration duration is 0
    // Last regen. distance is 0

    // delay(5000);
}
