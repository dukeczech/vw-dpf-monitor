#include "graphic.h"

#include <Arduino_GFX.h>
#include <math.h>

#include "bitmaps/arrows_icon.h"
#include "bitmaps/bt_icon.h"
#include "bitmaps/dpf_icon.h"
#include "bitmaps/fire_icon.h"
#include "config.h"

double ColorUtils::hue2rgb(double p, double q, double t) {
    if (t < 0) t += 1.0;
    if (t > 1) t -= 1.0;
    if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;
    if (t < 1.0 / 2.0) return q;
    if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
    return p;
}

int32_t ColorUtils::hsl2rgb(double hue, double sat, double light) {
    double r = 0.0, g = 0.0, b = 0.0;

    if (sat == 0) {
        r = g = b = light;  // achromatic
    } else {
        const double q =
            light < 0.5 ? light * (1.0 + sat) : light + sat - light * sat;
        const double p = 2.0 * light - q;

        r = hue2rgb(p, q, hue + 1.0 / 3.0);
        g = hue2rgb(p, q, hue);
        b = hue2rgb(p, q, hue - 1.0 / 3.0);
    }

    return ((int32_t)floor(r * 255)) << 16 | ((int32_t)floor(g * 255)) << 8 |
           ((int32_t)floor(b * 255));
}

double ColorUtils::percentage2hsl(double percentage, double hue0, double hue1) {
    return (percentage * (hue1 - hue0)) + hue0;
}

int32_t ColorUtils::number2hsl(double x, double min, double max) {
    double ratio = x;
    if (min > 0.0 || max < 1.0) {
        if (x < min) {
            ratio = 0.0;
        } else if (x > max) {
            ratio = 1.0;
        } else {
            const double range = max - min;
            ratio = (x - min) / range;
        }
    }

    // As the function expects a value between 0 and 1, and red = 0° and green =
    // 120° Convert the input to the appropriate hue value double hue = ratio * 1.2 / 3.60;
    const double hue = percentage2hsl(x, 40.0, 160.0) / 360.0;

    // Hue is value between 0 - 1
    // Convert hsl to rgb (saturation 100%, lightness 50%)
    return hsl2rgb(hue, 1.0, 0.45);
}

int32_t ColorUtils::number2rgb(double x) {
    // Calculate red and green
    const int32_t red = floor(255 - (255 * x));
    const int32_t green = floor(255 * x);

    return (red & 0xff) << 16 | (green & 0xff) << 8;
}

const GFXfont* Fonts::getFont(const uint8_t textsize, const bool bold) {
    switch (textsize) {
        case 1:
            return bold ? FONT_SIZE_8_BOLD : FONT_SIZE_8;
        case 2:
            return bold ? FONT_SIZE_10_BOLD : FONT_SIZE_10;
        case 3:
            return bold ? FONT_SIZE_12_BOLD : FONT_SIZE_12;
        case 4:
            return bold ? FONT_SIZE_14_BOLD : FONT_SIZE_14;
        case 5:
            return bold ? FONT_SIZE_16_BOLD : FONT_SIZE_16;
        default:
            return bold ? FONT_SIZE_12_BOLD : FONT_SIZE_12;
    }
}

const uint8_t Fonts::getHeight(const uint8_t textsize, const bool bold) {
    const GFXfont* font = Fonts::getFont(textsize, bold);

    uint8_t maxh = 0;
    for (size_t i = 0; i < (font->last - font->first); i++) {
        if (font->glyph[i].height > maxh) maxh = font->glyph[i].height;
    }
    return maxh;
}

Label::Label() {
    m_type = NONE;
    m_color = WHITE;
    m_paddingtop = 0;
    m_paddingbottom = 0;
    m_lastx = 0;
    m_lasty = 0;
    m_lastw = 0;
    m_lasth = 0;
}

Label::eLabelType Label::getType() const {
    return m_type;
}

uint16_t Label::getHeight(Arduino_GFX& gfx) const {
    return 0;
}

uint16_t Label::getWidth(Arduino_GFX& gfx) const {
    return 0;
}

Label& Label::setColor(const uint16_t color) {
    m_color = color;
    return *this;
}

Label& Label::setPadding(const int16_t top, const int16_t bottom) {
    m_paddingtop = top;
    m_paddingbottom = bottom;
    return *this;
}

uint16_t Label::draw(Arduino_GFX& gfx, const uint16_t x, const uint16_t y) {
    return 0;
}

const uint8_t TextLabel::DEFAULT_TEXT_SIZE = 2;

TextLabel::TextLabel() : Label() {
    m_type = TEXT;
    m_text = "";
    m_unit = "";
    m_textsize = DEFAULT_TEXT_SIZE;
}

TextLabel::TextLabel(const String& text, const uint8_t textsize) : Label() {
    m_type = TEXT;
    m_text = text;
    m_unit = "";
    m_textsize = textsize;
}

TextLabel::TextLabel(const String& text, const String& unit, const uint8_t textsize) : Label() {
    m_type = TEXT;
    m_text = text;
    m_unit = unit;
    m_textsize = textsize;
}

uint16_t TextLabel::getHeight(Arduino_GFX& gfx) const {
    int16_t x = 0, y = 0;
    uint16_t th = 0, tw = 0;

    gfx.getTextBounds(m_text + m_unit, 0, 0, &x, &y, &tw, &th);
    return th;
}

uint16_t TextLabel::getWidth(Arduino_GFX& gfx) const {
    int16_t x = 0, y = 0;
    uint16_t th = 0, tw = 0;

    gfx.getTextBounds(m_text + m_unit, 0, 0, &x, &y, &tw, &th);
    return tw;
}

const String& TextLabel::getText() const {
    return m_text;
}

const String& TextLabel::getUnit() const {
    return m_unit;
}

uint8_t TextLabel::getTextSize() const {
    return m_textsize;
}

TextLabel& TextLabel::setText(const String& text) {
    m_text = text;
    return *this;
}

TextLabel& TextLabel::setUnit(const String& unit) {
    m_unit = unit;
    return *this;
}

TextLabel& TextLabel::setTextSize(const uint8_t textsize) {
    m_textsize = textsize;
    return *this;
}

uint16_t TextLabel::draw(Arduino_GFX& gfx, const uint16_t x, const uint16_t y) {
    // Print the label in the centre of the cell width
    gfx.setFont(Fonts::getFont(m_textsize));
    gfx.setTextColor(m_color);

    const uint8_t hf = Fonts::getHeight(m_textsize);
    int16_t xr = 0, yr = 0;
    uint16_t th = 0, tw = 0;

    gfx.setTextWrap(false);
    gfx.getTextBounds(m_text + m_unit, x, y, &xr, &yr, &tw, &th);

    // Clear the background behind the label
    if (m_lastw > 0 && m_lasth > 0) gfx.fillRect(m_lastx, m_lasty, m_lastw, m_lasth, BACKGROUND_COLOR);

#ifdef USE_DEFAULT_FONT
    gfx.setCursor(x - tw / 2, y);
#else
    gfx.setCursor(x - tw / 2, y + hf + m_paddingtop);
#endif
    gfx.printf("%s%s", m_text.c_str(), m_unit.c_str());

    // Top left corner
    m_lastx = xr - tw / 2;
    m_lasty = yr + hf + m_paddingtop;
    m_lastw = tw;
    m_lasth = hf;
    return hf + m_paddingtop + m_paddingbottom;
}

ImageLabel::ImageLabel(const tImage& image) : Label(), m_image(image) {
    m_type = TEXT;
}

uint16_t ImageLabel::getHeight(Arduino_GFX& gfx) const {
    return m_image.height;
}

uint16_t ImageLabel::getWidth(Arduino_GFX& gfx) const {
    return m_image.width;
}

const tImage& ImageLabel::getImage() const {
    return m_image;
}

uint16_t ImageLabel::draw(Arduino_GFX& gfx, const uint16_t x, const uint16_t y) {
    // Print the label in the centre of the cell width
    gfx.drawBitmap(x - m_image.width / 2, y + m_paddingtop, m_image.data, m_image.width, m_image.height, m_color);
    return m_image.height + m_paddingtop + m_paddingbottom;
}

StatusBar::StatusBar(Arduino_GFX& gfx, const uint16_t h) : m_gfx(gfx) {
    m_h = h;
    m_borderh = 3;
    m_color = WHITE;
    m_bt = new BTIcon(gfx);
    m_comm = new CommIcon(gfx);
}

StatusBar::~StatusBar() {
    delete m_bt;
    delete m_comm;
}

uint16_t StatusBar::getHeight() const {
    return m_h;
}

uint16_t StatusBar::getBorderHeight() const {
    return m_borderh;
}

StatusBar& StatusBar::setText(const String& text, const uint16_t x) {
    m_text = text;
    m_textx = x;
    return *this;
}

void StatusBar::display() {
    // Put the border if needed
    m_gfx.fillRect(0, m_h - m_borderh, m_gfx.width(), m_borderh, m_color);

    // Put the text if needed
    m_gfx.setFont(Fonts::getFont(3));
    m_gfx.setTextColor(m_color);

    int16_t x1 = 0, y1 = 0;
    uint16_t th1 = 0, tw1 = 0;

    m_gfx.getTextBounds(m_text, 0, 0, &x1, &y1, &tw1, &th1);
#ifdef USE_DEFAULT_FONT
    m_gfx.setCursor(0, m_h - m_borderh - (m_h - m_borderh - th1) / 2);
#else
    m_gfx.setCursor(0 + m_textx, m_h - m_borderh - (m_h - m_borderh - th1) / 2);
#endif
    m_gfx.printf("%s", m_text.c_str());
}

ProgressBar::ProgressBar(Arduino_GFX& gfx, const uint16_t h) : m_gfx(gfx) {
    m_h = h;
    m_progress = 0.0;
}

ProgressBar::~ProgressBar() {}

uint16_t ProgressBar::getHeight() const {
    return m_h;
}

ProgressBar& ProgressBar::setProgress(const double& progress) {
    m_progress = progress;
    return *this;
}

void ProgressBar::display(const bool text) {
    const int16_t h = m_gfx.height();
    const int16_t w = m_gfx.width();

    const int32_t rgb = ColorUtils::number2rgb((100.0 - m_progress) / 100.0);
    const int32_t hsl = ColorUtils::number2hsl((100.0 - m_progress) / 100.0);
    const int16_t wb = ((w / 100.0) * m_progress);

    // Draw the progress
    m_gfx.fillRect(0, h - m_h - 3, w, m_h + 3, BACKGROUND_COLOR);
    m_gfx.fillRect(0, h - m_h, wb, m_h, m_gfx.color565((uint8_t)((hsl & 0xff0000) >> 16), (uint8_t)((hsl & 0xff00) >> 8), (uint8_t)(hsl & 0xff)));

    if (text) {
        String label = String(m_progress, 0);

        m_gfx.setFont(Fonts::getFont(2));
        m_gfx.setTextColor(WHITE);

        int16_t x1 = 0, y1 = 0;
        uint16_t th1 = 0, tw1 = 0;

        m_gfx.getTextBounds(label, 0, 0, &x1, &y1, &tw1, &th1);
#ifdef USE_DEFAULT_FONT
        m_gfx.setCursor(h / 2 - tw1 / 2, h - th1 - 3);
#else
        m_gfx.setCursor(w / 2 - tw1 / 2, h - 3);
#endif
        m_gfx.printf("%s%%", label.c_str());
    }
}

Cell::Cell(Arduino_GFX& gfx) : m_gfx(gfx) {
    m_x = 0;
    m_y = 0;
    m_w = 0;
    m_h = 0;
    m_borderwidth = 0;
    m_color = WHITE;
    m_label = NULL;
    m_labelallocated = false;
    m_value = NULL;
    m_valueallocated = false;
    m_unit = "";
    m_unitsize = TextLabel::DEFAULT_TEXT_SIZE;
    m_enabled = true;
}

Cell::~Cell() {
    if (m_labelallocated) delete m_label;
    if (m_valueallocated) delete m_value;
}

uint16_t Cell::getX() const {
    return m_x;
}

uint16_t Cell::getY() const {
    return m_y;
}

uint16_t Cell::getWidth() const {
    return m_w;
}

uint16_t Cell::getHeight() const {
    return m_h;
}

Label* Cell::getLabel() const {
    return m_label;
}

Label* Cell::getValue() const {
    return m_value;
}

void Cell::enable() {
    m_enabled = true;
}

void Cell::disable() {
    m_enabled = false;
}

Cell& Cell::setPosition(const uint16_t x, const uint16_t y) {
    m_x = x;
    m_y = y;
    return *this;
}

Cell& Cell::setSize(const uint16_t w, const uint16_t h) {
    m_w = w;
    m_h = h;
    return *this;
}

Cell& Cell::setColor(const uint16_t color) {
    m_color = color;
    return *this;
}

Cell& Cell::setLabel(const String& text, const uint8_t size) {
    if (m_labelallocated) delete m_label;

    m_label = new TextLabel(text, size);
    m_labelallocated = true;
    return *this;
}

Cell& Cell::setLabel(Label* lbl) {
    if (m_labelallocated) delete m_label;

    m_label = lbl;
    m_labelallocated = false;
    return *this;
}

Cell& Cell::setValue(const String& text, const uint8_t size) {
    if (m_valueallocated) delete m_value;

    m_value = new TextLabel(text, size);
    m_valueallocated = true;
    return *this;
}

Cell& Cell::setValue(Label* val) {
    if (m_valueallocated) delete m_value;

    m_value = val;
    m_valueallocated = false;
    return *this;
}

Cell& Cell::setUnit(const String& text, const uint8_t size) {
    m_unit = text;
    m_unitsize = size;
    return *this;
}

Cell& Cell::setBorder(const uint8_t w) {
    m_borderwidth = w;
    return *this;
}

void Cell::display(const bool repaint) {
    // Repaint the background if needed
    if (repaint || !m_enabled) {
        m_gfx.fillRect(m_x, m_y, m_w, m_h, BACKGROUND_COLOR);
    }

    if (m_enabled) {
        // Put the border if needed
        if (m_borderwidth > 0) {
            for (uint8_t i = 0; i < m_borderwidth; i++) {
                m_gfx.drawRect(m_x + i, m_y + i, m_w - 2 * i, m_h - 2 * i, m_color);
            }
        }

        m_gfx.setTextColor(m_color);

        uint16_t ycurr = 0;

        // Print the label
        if (m_label != NULL) {
            ycurr += m_label->draw(m_gfx, m_x + m_w / 2, m_y + ycurr + m_borderwidth);
        }

        // Print the value + unit
        if (m_value != NULL) {
            ycurr += m_value->draw(m_gfx, m_x + m_w / 2, m_y + ycurr + m_borderwidth);
        }
    }
}

const uint8_t Icon::ICON_PADDING_BOTTOM = 8;

Icon::Icon(Arduino_GFX& gfx, const tImage& image) : m_gfx(gfx), m_image(image), ImageLabel(image) {
    m_x = 0;
    m_y = 0;
    m_color = WHITE;
}

uint16_t Icon::getWidth(Arduino_GFX& gfx) const {
    return m_image.width;
}

uint16_t Icon::getHeight(Arduino_GFX& gfx) const {
    return m_image.height;
}

Icon& Icon::setPosition(const uint16_t x, const uint16_t y) {
    m_x = x;
    m_y = y;
    return *this;
}

void Icon::display() {
    display(m_color);
}

void Icon::display(const uint16_t color) {
    m_gfx.drawBitmap(m_x, m_y, m_image.data, m_image.width, m_image.height, color);
}

DPFIcon::DPFIcon(Arduino_GFX& gfx) : m_gfx(gfx), ImageLabel(dpf_icon) {
    m_label = TextLabel("---.-", "`C", 4);
    m_label.setPadding(6, 0);
}

ImageLabel& DPFIcon::getLabel() {
    return *this;
}

TextLabel& DPFIcon::getValue() {
    return m_label;
}

DPFIcon& DPFIcon::setTemperature(const double temp) {
    // Temp could be between 0 to 650
    m_label.setText(String(temp, 1));

    const double percent = 1.0 - (min(temp, 650.0) / 650.0);

    // You need to linearly interpolate (LERP) the color components. Here's how
    // it's done in general, given a start value v0, an end value v1 and the
    // required ratio (a normalized float between 0.0 and 1.0): v = v0 + ratio *
    // (v1 - v0) This gives v0 when ratio is 0.0, v1 when ratio is 1.0, and
    // everything between in the other cases.

    const double hue = ColorUtils::percentage2hsl(percent, 0.0, 100.0) / 360.0;
    // Hue is value between 0 - 1
    // Convert hsl to rgb (saturation 100%, lightness 50%)
    const int32_t rgb = ColorUtils::hsl2rgb(hue, 1.0, 0.6);

    m_color = m_gfx.color565((uint8_t)((rgb & 0xff0000) >> 16), (uint8_t)((rgb & 0xff00) >> 8), (uint8_t)(rgb & 0xff));
    m_label.setColor(m_color);
    return *this;
}

DPFIcon& DPFIcon::setTextSize(const uint8_t size) {
    m_label.setTextSize(size);
    return *this;
}

uint16_t DPFIcon::draw(Arduino_GFX& gfx, const uint16_t x, const uint16_t y) {
    // Draw the image label the default way
    uint16_t height = ImageLabel::draw(gfx, x, y);
    // Draw the text label the default way
    height += m_label.draw(gfx, x, y + height);
    return height;
}

BTIcon::BTIcon(Arduino_GFX& gfx) : Icon(gfx, bt_icon) {
    disable();
}

BTIcon& BTIcon::enable() {
    m_color = BLUE;
    return *this;
}

BTIcon& BTIcon::disable() {
    m_color = RED;
    return *this;
}

CommIcon::CommIcon(Arduino_GFX& gfx) : Icon(gfx, arrows_icon) {
    disable();
}

CommIcon& CommIcon::enable() {
    m_color = RGB565(0xFF, 0xFF, 0x66);
    return *this;
}

CommIcon& CommIcon::disable() {
    m_color = RGB565(0xC0, 0xC0, 0xC0);
    return *this;
}

FireIcon::FireIcon(Arduino_GFX& gfx) : Icon(gfx, fire_icon) {
    disable();
}

FireIcon& FireIcon::enable() {
    m_color = RGB565(0xFF, 0x00, 0x00);
    return *this;
}

FireIcon& FireIcon::disable() {
    m_color = BACKGROUND_COLOR;
    return *this;
}
