#pragma once

#define BACKGROUND_COLOR DARKCYAN

extern bool testMode;

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
#define TEST_MODE 1
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

#if 0
// TFT_eSPI GFX
#include <TFT_eSPI.h>

class TFT : public TFT_eSPI {
   public:
    TFT() : TFT_eSPI() {}

    void setFont(const GFXfont* f) {
        setFreeFont(f);
    }

    void getTextBounds(const String& input, int16_t x, int16_t y,
                       int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h) {
        uint8_t c;  // Current character

        *x1 = x;
        *y1 = y;
        *w = *h = 0;

        int16_t minx = this->width(), miny = this->width(), maxx = -1, maxy = -1;

        const char* str = input.c_str();
        while ((c = *str++))
            charBounds(c, &x, &y, &minx, &miny, &maxx, &maxy);

        if (maxx >= minx) {
            *x1 = minx;
            *w = maxx - minx + 1;
        }
        if (maxy >= miny) {
            *y1 = miny;
            *h = maxy - miny + 1;
        }
    }

    void charBounds(char c, int16_t* x, int16_t* y,
                    int16_t* minx, int16_t* miny, int16_t* maxx, int16_t* maxy) {
        if (gfxFont) {  // If non-default font is used not usable in my quick "hack"

            if (c == '\n')  // Newline
            {
                *x = _min_text_x;  // Reset x to zero, advance y by one line
                *y += (int16_t)textsize_y * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
            } else if (c != '\r')  // Not a carriage return; is normal char
            {
                uint8_t first = pgm_read_byte(&gfxFont->first),
                        last = pgm_read_byte(&gfxFont->last);
                if ((c >= first) && (c <= last))  // Char present in this font?
                {
                    GFXglyph* glyph = pgm_read_glyph_ptr(gfxFont, c - first);
                    uint8_t gw = pgm_read_byte(&glyph->width),
                            gh = pgm_read_byte(&glyph->height),
                            xa = pgm_read_byte(&glyph->xAdvance);
                    int8_t xo = pgm_read_sbyte(&glyph->xOffset),
                           yo = pgm_read_sbyte(&glyph->yOffset);
                    if (wrap && ((*x + ((xo + gw) * textsize_x) - 1) > _max_text_x)) {
                        *x = _min_text_x;  // Reset x to zero, advance y by one line
                        *y += (int16_t)textsize_y * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
                    }
                    int16_t x1 = *x + ((int16_t)xo * textsize_x),
                            y1 = *y + ((int16_t)yo * textsize_y),
                            x2 = x1 + ((int16_t)gw * textsize_x) - 1,
                            y2 = y1 + ((int16_t)gh * textsize_y) - 1;
                    if (x1 < *minx) {
                        *minx = x1;
                    }
                    if (y1 < *miny) {
                        *miny = y1;
                    }
                    if (x2 > *maxx) {
                        *maxx = x2;
                    }
                    if (y2 > *maxy) {
                        *maxy = y2;
                    }
                    *x += (int16_t)textsize_x * xa;
                }
            }
        } else {  // Default font

            if (c == '\n') {                                                          // Newline?
                *x = 0;                                                               // Reset x to zero,
                *y += this->textsize * 8;                                             // advance y one line
                                                                                      // min/max x/y unchaged -- that waits for next 'normal' character
            } else if (c != '\r') {                                                   // Normal char; ignore carriage returns
                if (/*wrap*/ false && ((*x + this->textsize * 6) > this->width())) {  // Off right?
                    *x = 0;                                                           // Reset x to zero,
                    *y += this->textsize * 8;                                         // advance y one line
                }
                int x2 = *x + this->textsize * 6 - 1,  // Lower-right pixel of char
                    y2 = *y + this->textsize * 8 - 1;
                if (x2 > *maxx) *maxx = x2;  // Track max x, y
                if (y2 > *maxy) *maxy = y2;
                if (*x < *minx) *minx = *x;  // Track min x, y
                if (*y < *miny) *miny = *y;
                *x += this->textsize * 6;  // Advance x one char
            }
        }
    }
};

using Arduino_GFX = TFT;

extern TFT gfx;
#endif

extern BTAdapter& serialBT;
