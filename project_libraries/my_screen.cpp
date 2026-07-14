#include "my_screen.h"

myScreen::myScreen() : frame_buffer(WIDTH * HEIGHT),
                       screen(WIDTH, HEIGHT, frame_buffer.data()),
                       st7789(WIDTH, HEIGHT, ROTATE_0, false, get_spi_pins(BG_SPI_FRONT)) {
    this->set_brightness(this->backlight);
    this->textx = 10;
    this->texty = 10;
    this->twidth = WIDTH - 20 - 10;
    this->clear();
}

//     this->textx = 10;
//     this->texty = 10;
//     this->twidth = WIDTH - 20 - 10;

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

void myScreen::clear(const std::string& color_name, int fade_steps, bool upd) {
    this->background_color = color_name;
    uint8_t target_color = Color::get_rgb332(color_name);

    // Immediate clear
    if (fade_steps <= 0) {
        screen.set_pen(target_color);
        this->rectangle(0, 0, WIDTH, HEIGHT);
        this->update();
        return;
    }

    auto step_towards_rgb332 = [](uint8_t from, uint8_t to) -> uint8_t {
        int from_r = (from >> 5) & 0x07;
        int from_g = (from >> 2) & 0x07;
        int from_b = from & 0x03;

        int to_r = (to >> 5) & 0x07;
        int to_g = (to >> 2) & 0x07;
        int to_b = to & 0x03;

        if (from_r < to_r) from_r++;
        else if (from_r > to_r) from_r--;

        if (from_g < to_g) from_g++;
        else if (from_g > to_g) from_g--;

        if (from_b < to_b) from_b++;
        else if (from_b > to_b) from_b--;

        return (from_r << 5) | (from_g << 2) | from_b;
    };

    const int fade_delay = 30;
    const int buffer_size = WIDTH * HEIGHT;

    for (int step = 0; step < fade_steps; step++) {
        for (int i = 0; i < buffer_size; i++) {
            frame_buffer[i] = step_towards_rgb332(
                frame_buffer[i],
                target_color
            );
        }

        this->update();
        sleep_ms(fade_delay);
    }

    // Ensure final color is exact
    screen.set_pen(target_color);
    this->rectangle(0, 0, WIDTH, HEIGHT);
    if (upd) this->update();
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

void myScreen::draw_logo(const std::string& title, const int steps, const int delay) {
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
    sleep_ms(delay*3);
    // Draw centered title under logo
    const int title_scale = 3;
    int text_width = screen.measure_text(title, title_scale);
    int text_x = (WIDTH - text_width) / 2;
    int text_y = y_spacing + (logo_height * scale) + (border * 2) + text_gap;

    // writexy adds +5 and +2 internally, so compensate here
    myScreen::writexy(text_x - 5, text_y - 2, title, "yellow", title_scale);
    this->update();
    sleep_ms(delay*5);

}

void myScreen::progress_bar(int segments) {
    const int bar_width = 120;
    const int bar_height = 10;
    const int total_segments = 12;
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
}

void myScreen::show_boot_message(std::string_view message, const std::string& color_name) {
    
    const int status_height = 16;
    const int status_x = 0;
    const int status_y = HEIGHT - 45;

    // Clear status area
    this->set_pen(background_color);
    this->rectangle(status_x, status_y, WIDTH, status_height);

    // Write status text
    if (!message.empty()) {
        this->set_pen(color_name);
        int text_width = screen.measure_text(message);
        int text_x = (WIDTH - text_width) / 2;
        screen.text(message, pimoroni::Point(text_x, status_y), WIDTH);
    }
    this->progress_bar();
    this->update();
    sleep_ms(500);
}

void myScreen::draw_buttonhint(int corner,
                               const std::string& color_name,
                               const Icons::Icon& icon) {
    const int radius = 24;

    int origin_x = 0;
    int origin_y = 0;

    switch (corner) {
        case 0: // top left
            origin_x = 0;
            origin_y = 0;
            break;

        case 1: // bottom left
            origin_x = 0;
            origin_y = HEIGHT - radius;
            break;

        case 2: // top right
            origin_x = WIDTH - radius;
            origin_y = 0;
            break;

        case 3: // bottom right
            origin_x = WIDTH - radius;
            origin_y = HEIGHT - radius;
            break;

        default:
            return;
    }

    // Draw the corner
    this->set_pen(color_name);

    for (int y = 0; y < radius; y++) {
        for (int x = 0; x < radius; x++) {
            int dx = x;
            int dy = y;

            if (corner == 1) {
                // bottom left: mirror Y
                dy = radius - 1 - y;
            } else if (corner == 2) {
                // top right: mirror X
                dx = radius - 1 - x;
            } else if (corner == 3) {
                // bottom right: mirror X and Y
                dx = radius - 1 - x;
                dy = radius - 1 - y;
            }

            if ((dx * dx + dy * dy) <= (radius * radius)) {
                this->rectangle(origin_x + x, origin_y + y, 1, 1);
            }
        }
    }

    // Icon inside the corner, hollow using background color
    const int icon_scale = 2;
    const int icon_width = icon.width * icon_scale;
    const int icon_height = icon.height * icon_scale;
    const int icon_margin = 2;

    int icon_x = origin_x + icon_margin;
    int icon_y = origin_y + icon_margin;

    switch (corner) {
        case 0: // top left
            icon_x = origin_x + icon_margin;
            icon_y = origin_y + icon_margin;
            break;

        case 1: // bottom left
            icon_x = origin_x + icon_margin;
            icon_y = origin_y + radius - icon_height - icon_margin;
            break;

        case 2: // top right
            icon_x = origin_x + radius - icon_width - icon_margin;
            icon_y = origin_y + icon_margin;
            break;

        case 3: // bottom right
            icon_x = origin_x + radius - icon_width - icon_margin;
            icon_y = origin_y + radius - icon_height - icon_margin;
            break;
    }

    this->set_pen(this->background_color);

    for (int row = 0; row < icon.height; row++) {
        uint8_t bits = icon.data[row];

        for (int col = 0; col < icon.width; col++) {
            bool pixel_on = bits & (1 << (7 - col));

            if (pixel_on) {
                this->rectangle(
                    icon_x + col * icon_scale,
                    icon_y + row * icon_scale,
                    icon_scale,
                    icon_scale
                );
            }
        }
    }
}

void myScreen::draw_clock_time(const std::string& clock_time,
                               const std::string& color_name,
                               int size) {
    // Expected format: "12:34"
    // Digits: 0-9
    // Blank: _
    // Separator: :
    if (clock_time.length() != 5) {
        return;
    }

    if (clock_time[2] != ':') {
        return;
    }

    const int thickness = size;
    const int length = size * 5;

    const int digit_width = thickness * 2 + length;
    const int digit_height = thickness * 3 + length * 2;

    const int digit_gap = size * 2;
    const int colon_width = size;
    const int colon_gap = size * 2;

    const int total_width =
        digit_width * 4 +
        digit_gap * 2 +
        colon_gap * 2 +
        colon_width;

    const int x_start = (WIDTH - total_width) / 2;

    // Keep clock below the top button hints
    const int buttonhint_radius = 24;
    const int buttonhint_gap = 20;

    const int y_start = buttonhint_radius + buttonhint_gap;

    // Extra margin around the clock when clearing its area
    const int clear_margin = size;

    auto draw_colon = [&](int x, int y) {
        const int dot_size = size;

        int upper_dot_y = y + digit_height / 3;
        int lower_dot_y = y + (digit_height * 2) / 3;

        time_t now = ::time(NULL);
        struct tm* timeinfo = gmtime(&now);

        bool show_colon = (timeinfo->tm_sec % 2) == 0;

        // Clear only the colon area
        this->set_pen(background_color);
        this->rectangle(x, y, colon_width, digit_height);

        // Draw colon only on even seconds
        if (show_colon) {
            this->set_pen(color_name);
            this->rectangle(x, upper_dot_y, dot_size, dot_size);
            this->rectangle(x, lower_dot_y, dot_size, dot_size);
        }
    };

    // If time has not changed, only update the colon
    if (clock_time == this->last_clock_time) {
        int colon_x = x_start
                    + digit_width + digit_gap
                    + digit_width + colon_gap;

        draw_colon(colon_x, y_start);

        this->update();
        return;
    }

    // Time has changed, so redraw only the clock area
    this->last_clock_time = clock_time;

    this->set_pen(background_color);
    this->rectangle(
        x_start - clear_margin,
        y_start - clear_margin,
        total_width + clear_margin * 2,
        digit_height + clear_margin * 2
    );

    this->set_pen(color_name);

    auto draw_segment = [&](int x, int y, char segment) {
        switch (segment) {
            case 'A':
                this->rectangle(
                    x + thickness,
                    y,
                    length,
                    thickness
                );
                break;

            case 'B':
                this->rectangle(
                    x + thickness + length,
                    y + thickness,
                    thickness,
                    length
                );
                break;

            case 'C':
                this->rectangle(
                    x + thickness + length,
                    y + thickness * 2 + length,
                    thickness,
                    length
                );
                break;

            case 'D':
                this->rectangle(
                    x + thickness,
                    y + thickness * 2 + length * 2,
                    length,
                    thickness
                );
                break;

            case 'E':
                this->rectangle(
                    x,
                    y + thickness * 2 + length,
                    thickness,
                    length
                );
                break;

            case 'F':
                this->rectangle(
                    x,
                    y + thickness,
                    thickness,
                    length
                );
                break;

            case 'G':
                this->rectangle(
                    x + thickness,
                    y + thickness + length,
                    length,
                    thickness
                );
                break;
        }
    };

    auto draw_digit = [&](int x, int y, char c) {
        if (c == '_') {
            return;
        }

        if (c < '0' || c > '9') {
            return;
        }

        int digit = c - '0';

        switch (digit) {
            case 0:
                draw_segment(x, y, 'A');
                draw_segment(x, y, 'B');
                draw_segment(x, y, 'C');
                draw_segment(x, y, 'D');
                draw_segment(x, y, 'E');
                draw_segment(x, y, 'F');
                break;

            case 1:
                draw_segment(x, y, 'B');
                draw_segment(x, y, 'C');
                break;

            case 2:
                draw_segment(x, y, 'A');
                draw_segment(x, y, 'B');
                draw_segment(x, y, 'G');
                draw_segment(x, y, 'E');
                draw_segment(x, y, 'D');
                break;

            case 3:
                draw_segment(x, y, 'A');
                draw_segment(x, y, 'B');
                draw_segment(x, y, 'G');
                draw_segment(x, y, 'C');
                draw_segment(x, y, 'D');
                break;

            case 4:
                draw_segment(x, y, 'F');
                draw_segment(x, y, 'G');
                draw_segment(x, y, 'B');
                draw_segment(x, y, 'C');
                break;

            case 5:
                draw_segment(x, y, 'A');
                draw_segment(x, y, 'F');
                draw_segment(x, y, 'G');
                draw_segment(x, y, 'C');
                draw_segment(x, y, 'D');
                break;

            case 6:
                draw_segment(x, y, 'A');
                draw_segment(x, y, 'F');
                draw_segment(x, y, 'G');
                draw_segment(x, y, 'E');
                draw_segment(x, y, 'C');
                draw_segment(x, y, 'D');
                break;

            case 7:
                draw_segment(x, y, 'A');
                draw_segment(x, y, 'B');
                draw_segment(x, y, 'C');
                break;

            case 8:
                draw_segment(x, y, 'A');
                draw_segment(x, y, 'B');
                draw_segment(x, y, 'C');
                draw_segment(x, y, 'D');
                draw_segment(x, y, 'E');
                draw_segment(x, y, 'F');
                draw_segment(x, y, 'G');
                break;

            case 9:
                draw_segment(x, y, 'A');
                draw_segment(x, y, 'B');
                draw_segment(x, y, 'C');
                draw_segment(x, y, 'D');
                draw_segment(x, y, 'F');
                draw_segment(x, y, 'G');
                break;
        }
    };

    int x = x_start;

    draw_digit(x, y_start, clock_time[0]);
    x += digit_width + digit_gap;

    draw_digit(x, y_start, clock_time[1]);
    x += digit_width + colon_gap;

    draw_colon(x, y_start);
    x += colon_width + colon_gap;

    draw_digit(x, y_start, clock_time[3]);
    x += digit_width + digit_gap;

    draw_digit(x, y_start, clock_time[4]);

    this->update();
}