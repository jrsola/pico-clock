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

void myScreen::writexy(int x, int y, const std::string_view &t, const std::string& color_name, int scale) {
    if (!color_name.empty()) {
        this->set_pen(color_name);
    }
    if (!t.empty()) {
        screen.text(t, pimoroni::Point(x + 5, y + 2), this->twidth, scale);
        this->update();
    }
}

void myScreen::draw_logo(const std::string& title) {
    const int scale = 2;
    const int border = 4;
    const int y_spacing = 20;
    const int text_gap = 8;

    const int logo_x = (WIDTH - (logo_width * scale)) / 2;
    const int logo_y = y_spacing;

    // draw the frame
    this->set_pen("grey");
    this->rectangle(
        logo_x - border,
        y_spacing - border, 
        (logo_width * scale) + (border * 2),
        (logo_height * scale) + (border * 2)
    );

    // fade-in logo
    const int steps = 15;
    const int delay = 100;

    for (int round = 0; round <= steps; round++){
        uint8_t brightness = (255 * round) / steps;
        for (int iy = 0; iy < logo_height; iy++) {
            for (int ix = 0; ix < logo_width; ix++) {
                int px = logo_x + ix * scale;
                int py = logo_y + iy * scale;
                // sets color from logo (current pixel)
                uint8_t color = logo[iy * logo_width + ix];
                uint8_t faded_color = Color::fade_rgb332(color, brightness);

                screen.set_pen(faded_color);
                this->rectangle(px, py, scale, scale);
            }
        }
        this->update();
        sleep_ms(delay);
    }

    // Draw centered title under logo
    const int title_scale = 3;
    int text_width = screen.measure_text(title, title_scale);
    int text_x = (WIDTH - text_width) / 2;
    int text_y = y_spacing + (logo_height * scale) + (border * 2) + text_gap;

    // writexy adds +5 and +2 internally, so compensate here
    myScreen::writexy(text_x - 5, text_y - 2, title, "yellow", title_scale);
}

void myScreen::progress_bar(int segments) {
    const int bar_width = 120;
    const int bar_height = 10;
    const int total_segments = 20;
    const int segment_width = 6;

    const int x = (WIDTH - bar_width) / 2;
    const int y = HEIGHT - bar_height - 5;

    // If an explicit value is passed, use it.
    // If no value is passed, add one segment.
    if (segments >= 0) {
        progress_segments = segments;
    } else {
        progress_segments++;
    }

    // Clamp/wrap
    if (progress_segments < 0) {
        progress_segments = 0;
    }

    if (progress_segments > total_segments) {
        progress_segments = 1;
    }

    // Draw empty bar
    this->set_pen("black");
    this->rectangle(x, y, bar_width, bar_height);

    // Draw progress segments
    this->set_pen("light green");

    for (int i = 0; i < progress_segments; i++) {
        int segment_x = x + i * segment_width;

        this->rectangle(
            segment_x,
            y,
            segment_width,
            bar_height
        );
    }

    this->update();
}

void myScreen::show_boot_status(std::string_view text, const std::string& color_name) {
    const int status_height = 16;
    const int status_x = 0;
    const int status_y = HEIGHT - 45;

    // Clear status area
    this->set_pen("black");
    this->rectangle(status_x, status_y, WIDTH, status_height);

    // Write status text
    if (!text.empty()) {
        this->set_pen(color_name);
        int text_width = screen.measure_text(text);
        int text_x = (WIDTH - text_width) / 2;
        screen.text(text, pimoroni::Point(text_x, status_y), WIDTH);
    }
    this->progress_bar();
    this->update();
    sleep_ms(500);
}
