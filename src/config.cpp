#include "config.h"

bool testMode = false;

#if BUILD_ENV_NAME == lolin32_lite
// Use specific code for Lolin-32 lite
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
BluetoothSerial serialBT;
#endif

#if BUILD_ENV_NAME == lilygo_t_display_s3
// Use specific code for T-display-s3

#if 1
Arduino_ESP32PAR8Q bus(7, 6, 8, 9, 39, 40, 41, 42, 45, 46, 47, 48);
Arduino_ST7789 gfx(&bus, 5, 0, true, 170, 320, 35, 0, 35, 0);
#endif

#if 0
TFT gfx = TFT();
#endif

BTAdapter& serialBT = BTAdapter::GetInstance();
#endif