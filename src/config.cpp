#include "config.h"

bool testMode = false;

// Use specific code for T-display-s3

#if 1
Arduino_ESP32PAR8Q bus(7, 6, 8, 9, 39, 40, 41, 42, 45, 46, 47, 48);
Arduino_ST7789 gfx(&bus, 5, 0, true, 170, 320, 35, 0, 35, 0);
#endif

#if 0
TFT gfx = TFT();
#endif

BTAdapter& serialBT = BTAdapter::GetInstance();
