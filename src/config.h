#include <Arduino_GFX.h>

#ifndef BUILD_ENV_NAME
#error "Add -D BUILD_ENV_NAME=$PIOENV to platformio.ini build_flags"
#else
#define lolin32_lite 101
#define lilygo_t_display_s3 102
#endif

#define BACKGROUND_COLOR DARKCYAN

extern bool testMode;

#if BUILD_ENV_NAME == lolin32_lite
// Use specific code for Lolin-32 lite

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BluetoothSerial.h>
#include <SPI.h>
#include <Wire.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

#define I2C_SDA 15
#define I2C_SCL 13

extern Adafruit_SSD1306 display;
extern BluetoothSerial serialBT;
#endif

#if BUILD_ENV_NAME == lilygo_t_display_s3
// Use specific code for T-display-s3
#include <Arduino_GFX_Library.h>

#include "bluetooth.h"

#define GFX_DEV_DEVICE LILYGO_T_DISPLAY_S3
#define GFX_EXTRA_PRE_INIT()              \
    {                                     \
        pinMode(15 /* PWD */, OUTPUT);    \
        digitalWrite(15 /* PWD */, HIGH); \
    }
#define GFX_BL 38
#define SIMPLE_GUI 0
#define TEST_MODE 1
#define RANDOM_DATA 1

extern Arduino_ESP32PAR8Q bus;
extern Arduino_ST7789 gfx;
extern BTAdapter& serialBT;
#endif