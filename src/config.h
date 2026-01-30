#pragma once

#define BACKGROUND_COLOR DARKCYAN

// Use specific code for T-display-s3
#if 0
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_ST77xx.h>
#include <gfxfont.h>
#endif

#include "bluetooth.h"

#define GFX_DEV_DEVICE LILYGO_T_DISPLAY_S3
#define GFX_EXTRA_PRE_INIT()              \
    {                                     \
        pinMode(15 /* PWD */, OUTPUT);    \
        digitalWrite(15 /* PWD */, HIGH); \
    }
#define GFX_BL 38

#define SIMPLE_GUI 0
#define TEST_MODE 0
#define RANDOM_DATA 1

#define RGB565(r, g, b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

#define RGB565_BLACK RGB565(0, 0, 0)
#define RGB565_NAVY RGB565(0, 0, 123)
#define RGB565_DARKGREEN RGB565(0, 125, 0)
#define RGB565_DARKCYAN RGB565(0, 125, 123)
#define RGB565_MAROON RGB565(123, 0, 0)
#define RGB565_PURPLE RGB565(123, 0, 123)
#define RGB565_OLIVE RGB565(123, 125, 0)
#define RGB565_LIGHTGREY RGB565(198, 195, 198)
#define RGB565_DARKGREY RGB565(123, 125, 123)
#define RGB565_BLUE RGB565(0, 0, 255)
#define RGB565_GREEN RGB565(0, 255, 0)
#define RGB565_CYAN RGB565(0, 255, 255)
#define RGB565_RED RGB565(255, 0, 0)
#define RGB565_MAGENTA RGB565(255, 0, 255)
#define RGB565_YELLOW RGB565(255, 255, 0)
#define RGB565_WHITE RGB565(255, 255, 255)
#define RGB565_ORANGE RGB565(255, 165, 0)
#define RGB565_GREENYELLOW RGB565(173, 255, 41)
#define RGB565_PALERED RGB565(255, 130, 198)

// Color definitions
#ifndef DISABLE_COLOR_DEFINES
#define BLACK RGB565_BLACK
#define NAVY RGB565_NAVY
#define DARKGREEN RGB565_DARKGREEN
#define DARKCYAN RGB565_DARKCYAN
#define MAROON RGB565_MAROON
#define PURPLE RGB565_PURPLE
#define OLIVE RGB565_OLIVE
#define LIGHTGREY RGB565_LIGHTGREY
#define DARKGREY RGB565_DARKGREY
#define BLUE RGB565_BLUE
#define GREEN RGB565_GREEN
#define CYAN RGB565_CYAN
#define RED RGB565_RED
#define MAGENTA RGB565_MAGENTA
#define YELLOW RGB565_YELLOW
#define WHITE RGB565_WHITE
#define ORANGE RGB565_ORANGE
#define GREENYELLOW RGB565_GREENYELLOW
#define PALERED RGB565_PALERED
#endif

#if 1
// Arduino GFX
#include <Arduino_GFX_Library.h>

extern Arduino_ESP32PAR8Q bus;
extern Arduino_ST7789 gfx;
#endif
#include <Preferences.h>

enum eRunningState {
    Init,
    Measuring,
    Idle,
    RegenerationEnd
};

struct WifiConfig {
    String ssid;
    String password;
};

struct MqttConfig {
    String server;
    uint16_t port;
    String username;
    String password;
    String topic;
};

extern bool testMode;
extern eRunningState state;
extern BTAdapter& serialBT;
extern Preferences prefs;
extern WifiConfig wificfg;
extern MqttConfig mqttcfg;

#define AP_SSID "DPF indicator"
#define AP_PASSWORD ""

class ApplicationConfig {
   public:
    static void load();
    static void save();
};
