#pragma once

#include <Arduino.h>

#include "graphic.h"

// First line
extern Cell cc1;
extern Cell cc2;
extern Cell cc3a;
extern Cell cc3b;

// Second line
extern Cell cc4;
extern Cell cc5;
extern Cell cc6;

extern DPFIcon dpfIcon;
extern BTIcon btIcon;
extern CommIcon commIcon;
extern WifiIcon wifiIcon;
extern FireIcon fireIcon;
extern StatusBar sb;
extern ProgressBar pb;

class Display {
   public:
    static bool m_dirty;
    static portMUX_TYPE m_mutex;

    Display();

    static bool init();
    static void lock();
    static void unlock();
    static void setDirty();

    // Setup all the graphic components
    static void setup();
    static void clear();
    static void display();

    static void welcome();

    static void regenerationEnd(const uint16_t topy, const double& time);
};