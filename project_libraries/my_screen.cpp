#include "my_screen.h"

// class constructor
PicoScreen::PicoScreen() : 
    PicoGraphics_PenRGB332(
        PicoDisplay2::WIDTH,
        PicoDisplay2::HEIGHT,
        nullptr // Pimoroni will create the framebuffer
    ),
    st7789(
        PicoDisplay2::WIDTH,
        PicoDisplay2::HEIGHT,
        ROTATE_0,
        false,
        get_spi_pins(BG_SPI_FRONT)
    ) 
{
    set_backlight(backlight);
    clear(Colors::BLACK);
}

// get the screen width
uint16_t PicoScreen::get_width() {
    return PicoDisplay2::WIDTH;
}

// get the screen height
uint16_t PicoScreen::get_height() {
    return PicoDisplay2::HEIGHT;
}

void PicoScreen::update() {
    st7789.update(this);
}

void PicoScreen::set_backlight(uint8_t backlight) {
    this->backlight = backlight; // save the value in our class attribute
    st7789.set_backlight(backlight);
}

uint8_t PicoScreen::get_backlight() {
    return backlight;
}

void PicoScreen::set_pen(const Colors::Color& color) {
    // save pen color for later use
    pen_color = color;
    PicoGraphics_PenRGB332::set_pen(color.r, color.g, color.b);
}

Colors::Color PicoScreen::get_pen() {
    return pen_color;
}

void PicoScreen::rectangle(int x, int y, int width, int height) {
    PicoGraphics_PenRGB332::rectangle(Rect(x, y, width, height));
}

void PicoScreen::clear(const Colors::Color& color, int fade_steps, bool upd) {
    background_color = color;
    
    // use target color as current pen
    set_pen(color);
    
    // immediate clear
    if (fade_steps <= 0) {
        rectangle(0, 0, get_width(), get_height());

        if(upd) update();
        return;
    }
    
    const int fade_delay = 30;
    const uint8_t alpha = 255 / fade_steps;

    for (int step = 0; step < fade_steps; step++) {
        for (int y = 0; y < get_height(); y++) {
            for (int x = 0; x < get_width(); x++){
                set_pixel_alpha(Point(x,y), alpha);
            }
        }
        if (upd) {
            update();
            sleep_ms(fade_delay);
        }
    }

    // get sure the final color is exactly the one intended
    rectangle(0, 0, get_width(), get_height());
 
    if (upd) update();
}

void PicoScreen::writexy(int x, int y, const std::string_view& t, const Colors::Color& color, int scale, bool upd) {
    if (t.empty()) {
        return;
    }

    set_pen(color);

    // write the text at (x,y), don't wrap it and with the scale specified
    text(t, Point(x, y), NO_WRAP, scale);
    if (upd) update();
}

void PicoScreen::draw_logo(const std::string& title, int delay) {
    const int scale = 2; // scale the image
    const int border = 4; // add a border around it
    const int y_spacing = 20; // start at this y position of the screen
    const int fading_steps = 15; // fade in steps for the logo

    // calculate the coordinates to place the logo centered on x
    const int logo_x = (get_width() - (logo_width * scale)) / 2;
    const int logo_y = y_spacing;

    // draw the border around the logo
    set_pen(Colors::GREY);
    rectangle(
        logo_x - border, 
        logo_y - border, 
        (logo_width * scale) + (border * 2), 
        (logo_height * scale) + (border * 2)
    );

    // fade logo in with alpha blending
    const uint8_t alpha = 255 / fading_steps;

    for (int step = 0; step < fading_steps; step++) {
        // draw the logo, one pixel and one row at a time
        for (int iy = 0; iy < logo_height; iy++) {
            for (int ix = 0; ix < logo_width; ix++) {
                const int px = logo_x + ix * scale;
                const int py = logo_y + iy * scale;

                // get current logo pixel color (already in RGB332)
                const uint8_t color = logo[iy * logo_width + ix];

                // use that logo color as current pen
                PicoGraphics_PenRGB332::set_pen(color);

                // draw a scale x scale block with alpha
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        set_pixel_alpha(Point(px + sx, py + sy), alpha);
                    }
                }
            }
        }

        update();
        sleep_ms(delay);
    }

    // draw the logo one final time with exact colors
    for (int iy = 0; iy < logo_height; iy++) {
        for (int ix = 0; ix < logo_width; ix++) {
            const int px = logo_x + ix * scale;
            const int py = logo_y + iy * scale;

            const uint8_t color = logo[iy * logo_width + ix];
            PicoGraphics_PenRGB332::set_pen(color);

            rectangle(px, py, scale, scale);
        }
    }

    update();
    sleep_ms(delay * 3);
    
    // draw text centered under logo (if provided)
    if (!title.empty()){
        const int text_gap = 8;
        const int title_scale = 3;
        
        // measure_text returns the width in pixels of a given text at a certain scale
        const int text_width = measure_text(title, title_scale);
        int text_x = (get_width() - text_width) / 2;
        int text_y = logo_y + (logo_height * scale) + (border * 2) + text_gap;

        writexy(text_x, text_y, title, Colors::YELLOW, title_scale, false);
        update();
    }

    sleep_ms(delay * 5);
}

void PicoScreen::draw_progress_bar(int segments) {
    const int bar_width = 120;
    const int bar_height = 10;

    const int x = (WIDTH - bar_width) / 2;
    const int y = HEIGHT - bar_height - 5;

    if (segments <= 0)
        return;

    const int segment_width = bar_width / segments;

    // adds one segment
    progress_segments++;

    // do not go over the max segments
    if (progress_segments > segments)
        progress_segments = segments;

    // deletes current bar
    this->set_pen("black");
    this->rectangle(x, y, bar_width, bar_height);

    // draw progress bar
    this->set_pen("light green");

    for (int i = 0; i < progress_segments; i++) {
        int width = segment_width;

        // last segment rounds up to remaning pixels
        if (i == segments - 1)
            width = bar_width - i * segment_width;

        this->rectangle(
            x + i * segment_width,
            y,
            width,
            bar_height
        );
    }
}

void PicoScreen::show_boot_message(std::string_view message, const Colors::Color& color) {
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

void PicoScreen::draw_clock_time(
    int x_start, 
    int y_start, 
    const std::string& clock_time, 
    const Colors::Color& color, 
    int size, 
    bool force_redraw
    ) {

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
    const int horizontal_length = size * 5;
    const int vertical_length = (horizontal_length * 7) / 4;

    const int digit_width = thickness * 2 + horizontal_length;
    const int digit_height = thickness * 3 + vertical_length * 2;

    const int digit_gap = size * 2;
    const int colon_width = size;
    const int colon_gap = size * 2;

    const int total_width =
        digit_width * 4 +
        digit_gap * 2 +
        colon_gap * 2 +
        colon_width;

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
            this->set_pen(color);
            this->rectangle(x, upper_dot_y, dot_size, dot_size);
            this->rectangle(x, lower_dot_y, dot_size, dot_size);
        }

        // Restore pen for following digits
        this->set_pen(color);
    };

    // If time has not changed, only update the colon
    if (!force_redraw && clock_time == this->last_clock_time) {
    int colon_x = x_start
                + digit_width + digit_gap
                + digit_width + colon_gap;

    draw_colon(colon_x, y_start);

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

    auto draw_segment = [&](int x, int y, char segment) {
        switch (segment) {
            case 'A':
                this->rectangle(
                    x + thickness,
                    y,
                    horizontal_length,
                    thickness
                );
                break;

            case 'B':
                this->rectangle(
                    x + thickness + horizontal_length,
                    y + thickness,
                    thickness,
                    vertical_length
                );
                break;

            case 'C':
                this->rectangle(
                    x + thickness + horizontal_length,
                    y + thickness * 2 + vertical_length,
                    thickness,
                    vertical_length
                );
                break;

            case 'D':
                this->rectangle(
                    x + thickness,
                    y + thickness * 2 + vertical_length * 2,
                    horizontal_length,
                    thickness
                );
                break;

            case 'E':
                this->rectangle(
                    x,
                    y + thickness * 2 + vertical_length,
                    thickness,
                    vertical_length
                );
                break;

            case 'F':
                this->rectangle(
                    x,
                    y + thickness,
                    thickness,
                    vertical_length
                );
                break;

            case 'G':
                this->rectangle(
                    x + thickness,
                    y + thickness + vertical_length,
                    horizontal_length,
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

    this->set_pen(color);

    draw_digit(x, y_start, clock_time[0]);
    x += digit_width + digit_gap;

    draw_digit(x, y_start, clock_time[1]);
    x += digit_width + colon_gap;

    draw_colon(x, y_start);
    x += colon_width + colon_gap;

    this->set_pen(color);

    draw_digit(x, y_start, clock_time[3]);
    x += digit_width + digit_gap;

    draw_digit(x, y_start, clock_time[4]);
}

void PicoScreen::draw_clock_time(
    const std::string& clock_time,
    const Colors::Color& color,
    int size,
    bool force_redraw
) {
    const int thickness = size;
    const int horizontal_length = size * 5;

    const int digit_width =
        thickness * 2 +
        horizontal_length;

    const int digit_gap = size * 2;
    const int colon_width = size;
    const int colon_gap = size * 2;

    const int total_width =
        digit_width * 4 +
        digit_gap * 2 +
        colon_gap * 2 +
        colon_width;

    const int x = (get_width() - total_width) / 2;
    const int y = 30;

    // És imprescindible transmetre force_redraw.
    draw_clock_time(
        x,
        y,
        clock_time,
        color,
        size,
        force_redraw
    );
}

// convert button label into a corner number for buttonhints
int button_to_corner(char button) {
    switch (button) {
        case 'a':
        case 'A':
            return 0; // top left

        case 'b':
        case 'B':
            return 1; // bottom left

        case 'x':
        case 'X':
            return 2; // top right

        case 'y':
        case 'Y':
            return 3; // bottom right

        default:
            return -1; // invalid button
    }
}

//draw one button hint
void PicoScreen::draw_buttonhints(
    const ActionIcons::ActionIcon& button_a,
    const ActionIcons::ActionIcon& button_b,
    const ActionIcons::ActionIcon& button_x,
    const ActionIcons::ActionIcon& button_y
) {
    const int radius = 24;

    int corner = button_to_corner(button);

    if (corner == -1) return;

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

    // clear current corner first
    this->set_pen(background_color);
    this->rectangle(origin_x, origin_y, radius, radius);

    // no icon assigned (action is None), nothign to draw
    if (action_icon.action = Action::None) {
        return;
    }

    // draw the full corner in the color specified
    this->set_pen(action_icon.color);

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

    // draw the icon inside the corner, using background color
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

void myScreen::default_buttonhints(){
    set_buttonhint('a', Icons::HEART, "yellow");
    set_buttonhint('b', Icons::CLOCK, "yellow");
    set_buttonhint('x', Icons::DISK, "yellow");
    set_buttonhint('y', Icons::INFO, "yellow");
    draw_buttonhints();
}

// draw all 4 button hints
void myScreen::draw_buttonhints(){
    clear_buttonhint('a');
    clear_buttonhint('b');
    clear_buttonhint('x');
    clear_buttonhint('y');
    draw_buttonhints();
}