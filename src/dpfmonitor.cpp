#include <Arduino.h>
#include <float.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <map>

#include "Every.h"
#include "config.h"
#include "gui/buttons.h"
#include "gui/display.h"
#include "gui/graphic.h"
#include "gui/sound.h"
#include "measurement.h"
#include "obd.h"
#include "server.h"
#include "storage.h"

// Interesting page https://github.com/blizniukp/WIFI_kit_32_dpf/tree/main
// https://github.com/jazdw/vag-blocks
// 067,3,Pressure Differential,DPF,Specification (Idle/Clean DPF): 3.0...7.0 mbar\nSpecification (Partitally clean DPF): 30.0...70.0 mbar\nSpecification (Fully Loaded DPF): max. 300.0 mbar

Every measureAction(5000);
Every btAction(1000);
Every wifiAction(1000);
Every summaryAction(8000);
Every::Timer beepAction(30000, false);
Every::Toggle statusAction(1000);

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

bool connect() {
    // Try the connection to be opened
    if (serialBT.IsConnected()) return true;

    (testMode) ? serialBT.Scan(2) : serialBT.Scan();

    serialBT.Loop();

    if (serialBT.IsConnected()) {
        Serial.println(F("Bluetooth channel is opened"));

        if (!myELM327.begin(serialBT, false, 2000)) {
            Serial.println(F("ELM327 begin failed"));

            return false;
        } else {
            Serial.println(F("ELM327 is connected"));
        }

        OBD::init();
        Serial.println(F("OBD init OK"));

        return true;
    }
    return false;
}

void setup() {
    state = Init;

    Serial.begin(115200);

    // Will wait for up to ~2 second for Serial to connect.
    delay(100);

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

    // Init the display
    GFX_EXTRA_PRE_INIT();

#ifdef GFX_BL
    pinMode(GFX_BL, OUTPUT);
    digitalWrite(GFX_BL, HIGH);
#endif

    Display::init();

    Display::welcome();

    Display::setup();

    idle();

    // Wifi AP web server init
    WifiServer::init("dpf-indicator", "");

    serialBT.SetCommunicationCallback(&callback);
    serialBT.Init();

    Serial.println(F("Bluetooth init end"));

    connect();

#if SIMPLE_GUI == 1
    gfx.setCursor(0, 16);
    gfx.println(F("Setup end..."));
#endif

    Display::display();

    Serial.println(F("DPF indicator setup end"));
}

void displayLine(uint8_t line, const char* label, const double value, const uint8_t precision, const char* unit) {
    gfx.fillRect(0, line * 16, gfx.width() - 10, 16, BLACK);
    gfx.setCursor(0, line * 16);
    gfx.printf("%s: %.*f %s\n", label, precision, value, unit);
}

void idle() {
    Display::lock();
    if (statusAction()) {
        sb.display();

        // Update regeneration status
        if (Regeneration::isRegenerating() || Buttons::isPressedDown())
            if (statusAction.state)
                fireIcon.enable().display();
            else
                fireIcon.disable().display();
        else
            fireIcon.disable().display();
    }
    if (wifiAction()) {
        // Update wifi status
        WifiServer::hasClient() ? wifiIcon.enable().display() : wifiIcon.disable().display();
    }
    Display::unlock();

    // Switch to the summary page if button is pressed
    if (Buttons::isPressedUp()) {
        Serial.println(F("Switch to summary page"));
        if (Display::getPage() == Display::MAIN) {
            Display::setPage(Display::SUMMARY);
            Display::clear();
            Display::setDirty();
            Display::display();
        }
        summaryAction.reset();
    }

    // Switch to the main page after the timeout
    if (summaryAction()) {
        if (Display::getPage() == Display::SUMMARY) {
            Serial.println(F("Switch to main page"));
            Display::setPage(Display::MAIN);
            Display::clear();
            Display::setDirty();
            Display::display();
        }
    }

    if (Regeneration::isRegenerating() || Buttons::isPressedDown()) {
        if (!beepAction.running) {
            beepAction.reset();
        } else {
            if (beepAction()) {
                // Regeneration is on-going
                Sound::beep1short();
                beepAction.reset();
            }
        }
    } else {
        beepAction.running = false;
    }
}

void loop() {
    // Check the file manager
#ifdef WITH_FILEMANAGER
    if (WifiServer::hasClient()) {
        FileManager::setupFilemanager();
    }
    FileManager::loop();
#endif

    if (state != RegenerationEnd) {
        state = Idle;
    }

    // Check the status every 1s
    if (btAction()) {
        if (!serialBT.IsConnected() && !testMode) {
            // Reconnect
            if (connect()) {
                Display::display();
            }
        }
    }

    // Process idle
    idle();

    // Iterate over measurements map every 5s
    bool measureOK = true;
    if (measureAction()) {
        if (state == RegenerationEnd) {
            measureAction.reset(5000, false);
            // Clear the display
            Display::clear();
        }
        state = Measuring;
        for (auto itr = Measurements::getActual().begin(); itr != Measurements::getActual().end(); ++itr) {
            if (itr->second.enabled) {
                // Check the connection status
                if (!serialBT.IsConnected() && !testMode) {
                    Serial.println(F("Bluetooth channel disconnected"));
                    measureOK = false;
                    break;
                }

                const bool result = Measurements::measure(itr->second, testMode);
                if (!result) {
                    // Measurement error
                    Serial.printf("Measurement error: %s\n", itr->second.caption);
                    measureOK = false;
                    break;
                }

                // Recalculate soot load
                if (itr->first == SOOT_MASS_CALCULATED) {
                    Measurements::getValue(Measurements::getActual(), SOOT_LOAD) =
                        min(Measurements::getValue(Measurements::getActual(), SOOT_MASS_CALCULATED) / 0.24, 100.0);
                }
            }
        }

        // Simulate the regeneration start
        if (false) {
            if (Regeneration::isRegenerating()) {
                // Stop the regeneration
                Measurements::setValue(Measurements::getLast(), REGENERATION_DURATION, 12.6);
                Measurements::setValue(Measurements::getLast(), DPF_INPUT_TEMPERATURE, 610.0);
                Measurements::setValue(Measurements::getLast(), POST_INJECTION_2, 3.66);
                Measurements::setValue(Measurements::getLast(), POST_INJECTION_3, 5.13);

                Measurements::setValue(Measurements::getActual(), REGENERATION_DURATION, 0.0);
                Measurements::setValue(Measurements::getActual(), DPF_INPUT_TEMPERATURE, 250.0);
                Measurements::setValue(Measurements::getActual(), POST_INJECTION_2, 0.0);
                Measurements::setValue(Measurements::getActual(), POST_INJECTION_3, 0.0);
            } else {
                // Start the regeneration
                Measurements::setValue(Measurements::getActual(), REGENERATION_DURATION, 0.1);
                Measurements::setValue(Measurements::getActual(), DPF_INPUT_TEMPERATURE, 400.0);

                Measurements::disableLog();
            }
        }

        // Check the first run and assign the measurements at beginning
        if (measureOK) {
            if (Measurements::getStart().empty()) {
                Measurements::getStart() = Measurements::getActual();
            }
        }

        //  Display the measurement data
        uint8_t line = 0;
        Serial.println("------------------------------------------");
        for (auto itr = Measurements::getActual().begin(); itr != Measurements::getActual().end(); ++itr) {
#if DEBUG_LOG == 1
            if (itr->second.enabled) {
                Serial.printf("%s: %.*f %s\n", itr->second.caption, itr->second.precision, itr->second.value, itr->second.unit);
#if SIMPLE_GUI == 1
                displayLine(line, itr->second.shortCaption, itr->second.value, itr->second.precision, itr->second.unit);
#endif
                line++;
                if (line >= 10) {
                    line = 0;
                }
            }
#endif
        }
        if (testMode) {
            Serial.print(Measurements::toString(REGENERATION_ON, Measurements::getActual()));
        }
        Serial.println("------------------------------------------");

        // Check the regeneration process
        if (Regeneration::check()) {
            if (state == RegenerationEnd) {
                // Start the timer to let the screen on
                measureAction.reset(8000, false);
                Display::setDirty();
                return;
            }
        }

        // Update the GUI values
        // Main page
        if (Measurements::isEnabled(Measurements::getActual(), DPF_INPUT_TEMPERATURE)) {
            dpfIcon.setTemperature(Measurements::getValue(Measurements::getActual(), DPF_INPUT_TEMPERATURE));
        }
        if (Measurements::isEnabled(Measurements::getActual(), SOOT_MASS_CALCULATED)) {
            const String& str = String(Measurements::getValue(Measurements::getActual(), SOOT_MASS_CALCULATED), 2);
            if (cc2.getValue() != NULL) ((TextLabel*)cc2.getValue())->setText(str);

            const double val = Measurements::diff(SOOT_MASS_CALCULATED);
            if (val >= 0.0) {
                if (cc5.getValue() != NULL) ((TextLabel*)cc5.getValue())->setText("(+" + String(val, 2));
            } else {
                if (cc5.getValue() != NULL) ((TextLabel*)cc5.getValue())->setText("(" + String(val, 2));
            }
        }
        if (Measurements::isEnabled(Measurements::getActual(), DISTANCE_SINCE_LAST_REGENERATION)) {
            const String& str = String(Measurements::getValue(Measurements::getActual(), DISTANCE_SINCE_LAST_REGENERATION), 1);
            if (cc3a.getValue() != NULL) ((TextLabel*)cc3a.getValue())->setText(str);

            const double val = Measurements::diff(DISTANCE_SINCE_LAST_REGENERATION);
            if (val >= 0.0) {
                if (cc6.getValue() != NULL) ((TextLabel*)cc6.getValue())->setText("(+" + String(val, 1));
            } else {
                if (cc6.getValue() != NULL) ((TextLabel*)cc6.getValue())->setText("(" + String(val, 1));
            }
        }
        if (Measurements::isEnabled(Measurements::getActual(), REGENERATION_DURATION)) {
            const String& str = String(Measurements::getValue(Measurements::getActual(), REGENERATION_DURATION), 1);
            if (cc3b.getValue() != NULL) ((TextLabel*)cc3b.getValue())->setText(str);
        }
        // Soot load is always disabled
        pb.setProgress(Measurements::getValue(Measurements::getActual(), SOOT_LOAD));

        // Summary page
        if (Measurements::isEnabled(Measurements::getActual(), OIL_ASH_RESIDUE)) {
            const String& str = String(Measurements::getValue(Measurements::getActual(), OIL_ASH_RESIDUE), 2);
            if (sp1.getValue() != NULL) ((TextLabel*)sp1.getValue())->setText(str);
        }

        // Update the GUI
        Display::display();

        // Update the last measurements
        if (measureOK) {
            // Assign the last values to the actual one
            Measurements::getLast() = Measurements::getActual();
        }
        state = Idle;
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
