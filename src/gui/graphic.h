#include <Arduino.h>
#include <Arduino_GFX.h>
#include <inttypes.h>

#include "bitmaps/icon_data.h"
#include "fonts/NotoSansMono10pt7b.h"
#include "fonts/NotoSansMono12pt7b.h"
#include "fonts/NotoSansMono14pt7b.h"
#include "fonts/NotoSansMono16pt7b.h"
#include "fonts/NotoSansMono8pt7b.h"
#include "fonts/NotoSansRegular10pt7b.h"
#include "fonts/NotoSansRegular12pt7b.h"
#include "fonts/NotoSansRegular14pt7b.h"
#include "fonts/NotoSansRegular16pt7b.h"
#include "fonts/NotoSansRegular8pt7b.h"

// #define USE_DEFAULT_FONT 1

#define FONT_SIZE_8 &NotoSansMono_Condensed_Bold8pt7b
#define FONT_SIZE_10 &NotoSansMono_Condensed_Bold10pt7b
#define FONT_SIZE_12 &NotoSansMono_Condensed_Bold12pt7b
#define FONT_SIZE_14 &NotoSansMono_Condensed_Bold14pt7b
#define FONT_SIZE_16 &NotoSansMono_Condensed_Bold16pt7b
#define FONT_SIZE_8_BOLD &NotoSansMono_Condensed_Bold8pt7b
#define FONT_SIZE_10_BOLD &NotoSansMono_Condensed_Bold10pt7b
#define FONT_SIZE_12_BOLD &NotoSansMono_Condensed_Bold12pt7b
#define FONT_SIZE_14_BOLD &NotoSansMono_Condensed_Bold14pt7b
#define FONT_SIZE_16_BOLD &NotoSansMono_Condensed_Bold16pt7b

class BTIcon;
class CommIcon;

class ColorUtils {
   public:
    static double hue2rgb(double p, double q, double t);
    static int32_t hsl2rgb(double hue, double sat, double light);

    /**
     * You define the range of your hue, by providing the 0 and 1 value as
     * parameters, the calculation of the hue value becomes basic math
     * https://stackoverflow.com/questions/17525215/calculate-color-values-from-green-to-red
     * @param percentage: a value between 0 and 1
     * @param hue0: the hue value of the color you want to get when the
     * percentage is 0
     * @param hue1: the hue value of the color you want to get when the
     * percentage is 1
     * @return value between hue0 - hue1
     */
    static double percentage2hsl(double percentage, double hue0, double hue1);

    /**
     * Convert a number to a color using hsl, with range definition.
     * Example: if min/max are 0/1, and i is 0.75, the color is closer to green.
     * Example: if min/max are 0.5/1, and i is 0.75, the color is in the middle
     * between red and green.
     * @param x (floating point, range 0 to 1)
     * @param min (floating point, range 0 to 1, all i at and below this is red)
     * @param max (floating point, range 0 to 1, all i at and above this is
     * green)
     */
    static int32_t number2hsl(double x, double min = 0.0, double max = 1.0);

    /**
     * Convert a number to a color using rgb
     * @param x (floating point, range 0 to 1)
     */
    static int32_t number2rgb(double x);
};

class Fonts {
   public:
    static const GFXfont* getFont(const uint8_t textsize, const bool bold = false);
    static const uint8_t getHeight(const uint8_t textsize, const bool bold = false);
};

class Label {
   public:
    enum eLabelType {
        NONE,
        TEXT,
        IMAGE
    };

    Label();

    virtual eLabelType getType() const;
    virtual uint16_t getHeight(Arduino_GFX& gfx) const = 0;
    virtual uint16_t getWidth(Arduino_GFX& gfx) const = 0;

    virtual Label& setColor(const uint16_t color);
    virtual Label& setPadding(const int16_t top, const int16_t bottom);

    virtual uint16_t draw(Arduino_GFX& gfx, const uint16_t x, const uint16_t y);

   protected:
    eLabelType m_type;
    uint16_t m_color;
    int16_t m_paddingtop;
    int16_t m_paddingbottom;
    uint16_t m_lastx;
    uint16_t m_lasty;
    uint16_t m_lastw;
    uint16_t m_lasth;
};

class TextLabel : public Label {
   public:
    static const uint8_t DEFAULT_TEXT_SIZE;

    TextLabel();
    TextLabel(const String& text, const uint8_t textsize);
    TextLabel(const String& text, const String& unit, const uint8_t textsize);

    virtual uint16_t getHeight(Arduino_GFX& gfx) const override;
    virtual uint16_t getWidth(Arduino_GFX& gfx) const override;
    virtual const String& getText() const;
    virtual const String& getUnit() const;
    virtual uint8_t getTextSize() const;

    virtual TextLabel& setText(const String& text);
    virtual TextLabel& setUnit(const String& unit);
    virtual TextLabel& setTextSize(const uint8_t textsize);

    virtual uint16_t draw(Arduino_GFX& gfx, const uint16_t x, const uint16_t y) override;

   protected:
    String m_text;
    String m_unit;
    uint8_t m_textsize;
};

class ImageLabel : public Label {
   public:
    ImageLabel(const tImage& image);

    virtual uint16_t getHeight(Arduino_GFX& gfx) const override;
    virtual uint16_t getWidth(Arduino_GFX& gfx) const override;
    virtual const tImage& getImage() const;

    virtual uint16_t draw(Arduino_GFX& gfx, const uint16_t x, const uint16_t y) override;

   protected:
    const tImage& m_image;
};

class StatusBar {
   public:
    enum eTextPosition {
        LEFT,
        CENTER,
        RIGHT
    };

    StatusBar(Arduino_GFX& gfx, const uint16_t h);
    virtual ~StatusBar();

    virtual uint16_t getHeight() const;
    virtual uint16_t getBorderHeight() const;

    virtual StatusBar& setText(const String& text, const uint16_t x = 0);

    virtual void display();

   protected:
    Arduino_GFX& m_gfx;
    uint16_t m_h;
    uint16_t m_borderh;
    uint16_t m_color;
    String m_text;
    uint16_t m_textx;
    BTIcon* m_bt;
    CommIcon* m_comm;
};

class ProgressBar {
   public:
    ProgressBar(Arduino_GFX& gfx, const uint16_t h);
    virtual ~ProgressBar();

    virtual uint16_t getHeight() const;

    virtual ProgressBar& setProgress(const double& progress);

    virtual void display(const bool text = false);

   protected:
    Arduino_GFX& m_gfx;
    uint16_t m_h;
    double m_progress;
};

class Cell {
   public:
    Cell(Arduino_GFX& gfx);
    virtual ~Cell();

    virtual uint16_t getX() const;
    virtual uint16_t getY() const;
    virtual uint16_t getWidth() const;
    virtual uint16_t getHeight() const;
    virtual Label* getLabel() const;
    virtual Label* getValue() const;

    virtual void enable();
    virtual void disable();

    virtual Cell& setPosition(const uint16_t x, const uint16_t y);
    virtual Cell& setSize(const uint16_t w, const uint16_t h);
    virtual Cell& setColor(const uint16_t color);

    virtual Cell& setLabel(const String& text, const uint8_t size = TextLabel::DEFAULT_TEXT_SIZE);
    virtual Cell& setLabel(Label* lbl);

    virtual Cell& setValue(const String& text, const uint8_t size = TextLabel::DEFAULT_TEXT_SIZE);
    virtual Cell& setValue(Label* val);

    virtual Cell& setUnit(const String& text, const uint8_t size = TextLabel::DEFAULT_TEXT_SIZE);
    virtual Cell& setBorder(const uint8_t w);

    virtual void display(const bool repaint = false);

   protected:
    Arduino_GFX& m_gfx;
    uint16_t m_x;
    uint16_t m_y;
    uint16_t m_w;
    uint16_t m_h;
    uint8_t m_borderwidth;
    uint16_t m_color;
    Label* m_label;
    bool m_labelallocated;
    Label* m_value;
    bool m_valueallocated;
    String m_unit;
    uint8_t m_unitsize;
    bool m_enabled;
};

class Icon : public ImageLabel {
   public:
    static const uint8_t ICON_PADDING_BOTTOM;

    Icon(Arduino_GFX& gfx, const tImage& image);

    virtual uint16_t getWidth(Arduino_GFX& gfx) const override;
    virtual uint16_t getHeight(Arduino_GFX& gfx) const override;

    virtual Icon& setPosition(const uint16_t x, const uint16_t y);

    virtual void display();
    virtual void display(const uint16_t color);

   protected:
    Arduino_GFX& m_gfx;
    uint16_t m_x;
    uint16_t m_y;
    const tImage& m_image;
    uint16_t m_color;
};

class DPFIcon : public ImageLabel {
   public:
    DPFIcon(Arduino_GFX& gfx);

    ImageLabel& getLabel();
    TextLabel& getValue();

    DPFIcon& setTemperature(const double temp);
    DPFIcon& setTextSize(const uint8_t size);

    virtual uint16_t draw(Arduino_GFX& gfx, const uint16_t x, const uint16_t y) override;

   protected:
    Arduino_GFX& m_gfx;
    TextLabel m_label;
};

class BTIcon : public Icon {
   public:
    BTIcon(Arduino_GFX& gfx);

    BTIcon& enable();
    BTIcon& disable();
};

class CommIcon : public Icon {
   public:
    CommIcon(Arduino_GFX& gfx);

    CommIcon& enable();
    CommIcon& disable();
};

class FireIcon : public Icon {
   public:
    FireIcon(Arduino_GFX& gfx);

    FireIcon& enable();
    FireIcon& disable();
};
