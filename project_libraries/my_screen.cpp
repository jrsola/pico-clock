#include "my_screen.h"

myScreen::myScreen() : frame_buffer(WIDTH * HEIGHT),
                       screen(WIDTH, HEIGHT, frame_buffer.data()),
                       st7789(WIDTH, HEIGHT, ROTATE_0, false, get_spi_pins(BG_SPI_FRONT)) {
    this->set_brightness(this->backlight);
    this->set_pen("white");
    this->clear();
    this->update();
    sleep_ms(2000);
}

uint16_t myScreen::get_width() {
    return WIDTH;
}

uint16_t myScreen::get_height() {
    return HEIGHT;
}

void myScreen::set_brightness(uint8_t backlight) {
    this->backlight = backlight;
    st7789.set_backlight(this->backlight);
}

uint8_t myScreen::get_brightness() {
    return this->backlight;
}

void myScreen::set_pen(const std::string& color_name) {
    auto [r, g, b] = Color::get_rgb(color_name);
    this->set_pen(r, g, b);
}

void myScreen::set_pen(std::tuple<uint8_t, uint8_t, uint8_t> rgb_tuple) {
    auto [r, g, b] = rgb_tuple;
    this->set_pen(r, g, b);
}

void myScreen::set_pen(uint8_t r, uint8_t g, uint8_t b) {
    this->pen_color = {r, g, b};
    screen.set_pen(r, g, b);
}

std::tuple<uint8_t, uint8_t, uint8_t> myScreen::get_pen() {
    return this->pen_color;
}

void myScreen::pixel(const Point &p) {
    screen.pixel(p);
}

void myScreen::clear() {
    this->textx = 10;
    this->texty = 10;
    this->twidth = WIDTH - 20 - 10;
    screen.clear();
}

void myScreen::update() {
    st7789.update(&screen);
}

void myScreen::rectangle(int x, int y, int width, int height) {
    screen.rectangle(Rect(x, y, width, height));
}

void myScreen::writeln(const std::string_view &t, const std::string& color_name) {
    this->writexy(this->textx, this->texty, t, color_name);
    this->textx = 10;
    this->texty += 16;
}

void myScreen::writexy(int x, int y, const std::string_view &t, const std::string& color_name) {
    if (!color_name.empty()) {
        this->set_pen(color_name);
    }
    if (!t.empty()) {
        screen.text(t, pimoroni::Point(x + 5, y + 2), this->twidth);
        this->update();
    }
}