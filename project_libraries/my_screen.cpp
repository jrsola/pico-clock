#include "my_screen.h"

// class constructor
PicoScreen::PicoScreen() : 
    PicoGraphics_PenRGB332(
        PicoDisplay2::WIDTH,
        PicoDisplay2::HEIGHT,
        nullptr // framebuffer for screen, will define later
    ),
    st7789(
        PicoDisplay2::WIDTH,
        PicoDisplay2::HEIGHT,
        ROTATE_0,
        false,
        get_spi_pins(BG_SPI_FRONT)
    ),
    // define a framebuffer with the screen's dimensions
    frame_buffer(PicoDisplay2::WIDTH * PicoDisplay2::HEIGHT)
{
    // now we assign the framebuffer
    set_framebuffer(frame_buffer.data());

    set_brightness(backlight);

    textx = 10;
    texty = 10;
    twidth = get_width() - 20 - 10;
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

// call the chipset to update the screen content
void PicoScreen::update() {
    st7789.update(&screen);
}

// call the chipset to set the screen brightness level
// save the value in our class attribute
void PicoScreen::set_brightness(uint8_t backlight) {
    this->backlight = backlight;
    st7789.set_backlight(this->backlight);
}

// retrieve the screen brightness 
uint8_t PicoScreen::get_brightness() {
    return backlight;
}

// set the pen color using Pimononi's library
// save the value in a class attribute
void PicoScreen::set_pen(const Colors::Color& color) {
    // save pen color for later use
    this->pen_color = color;
    PicoGraphics_PenRGB332::set_pen(color.r, color.g, color.b);
}

// retrieve the pen color
Colors::Color PicoScreen::get_pen() {
    return pen_color;
}

// just calling the standard rectangle method wihtout a Rect object
void PicoScreen::rectangle(int x, int y, int width, int height) {
    PicoGraphics_PenRGB332::rectangle(Rect(x, y, width, height));
}

// clear the screen, i.e. fill it all with a background color
// optionally we can add a fade effect (default is no fading)
// screen can be kept without updating (default is update)
void PicoScreen::clear(const Colors::Color& color, int fade_steps, bool upd) {
    background_color = color;
    uint8_t target_color = Colors::to_rgb332(color);

    // immediate clear
    if (fade_steps <= 0) {
        PicoGraphics_PenRGB332::set_pen(target_color);
        
        rectangle(0, 0, get_width(), get_height());

        if(upd) update();
        return;
    }

    const int fade_delay = 30;
    const int buffer_size = get_width() * get_height();

    for (int step = 0; step < fade_steps; step++) {
        for (int i = 0; i < buffer_size; i++) {
            frame_buffer[i] = Colors::transition_to_rgb332(frame_buffer[i], target_color);
    }

        update();
        sleep_ms(fade_delay);
    }

    // get sure the final color is exactly the one stated
    PicoGraphics_PenRGB332::set_pen(target_color);
    
    rectangle(0, 0, get_width(), get_height());
 
    if (upd) update();
}

void PicoScreen::writexy(int x, int y, const std::string_view &t, const Colors::Color& color, int scale) {
    if (t.empty()) {
        return;
    }

    set_pen(color);

    text(t, Point(x + 5, y + 2), twidth, scale);
    update();

}

// draw a fading bootup logo using the image in logo_rgb332.h
// optionally can show a text under the logo (default is none), 
// and control the speed (default is 100 ms for each fading step)
void PicoScreen::draw_logo(const std::string& title, const int delay) {
    const int scale = 2;
    const int border = 4;
    const int y_spacing = 20;

    const int logo_x = (get_width() - (logo_width * scale)) / 2;
    const int logo_y = y_spacing;

    // draw the frame
    this->set_pen("grey");
    this->rectangle(
        logo_x - border,
        y_spacing - border, 
        (logo_width * scale) + (border * 2),
        (logo_height * scale) + (border * 2)
    );

    const int steps = 15;
    for (int round = 0; round <= steps; round++)
    {
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
    const int text_gap = 8;
    const int title_scale = 3;
    int text_width = screen.measure_text(title, title_scale);
    int text_x = (get_width() - text_width) / 2;
    int text_y = y_spacing + (logo_height * scale) + (border * 2) + text_gap;

    // writexy adds +5 and +2 internally, so compensate here
    myScreen::writexy(text_x - 5, text_y - 2, title, "yellow", title_scale);
    this->update();
    sleep_ms(delay*5);

}

void myScreen::progress_bar(int segments) {
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

void myScreen::draw_clock_time(int x_start, int y_start, const std::string& clock_time, const std::string& color_name, int size, bool force_redraw) {

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
            this->set_pen(color_name);
            this->rectangle(x, upper_dot_y, dot_size, dot_size);
            this->rectangle(x, lower_dot_y, dot_size, dot_size);
        }

        // Restore pen for following digits
        this->set_pen(color_name);
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

    this->set_pen(color_name);

    draw_digit(x, y_start, clock_time[0]);
    x += digit_width + digit_gap;

    draw_digit(x, y_start, clock_time[1]);
    x += digit_width + colon_gap;

    draw_colon(x, y_start);
    x += colon_width + colon_gap;

    this->set_pen(color_name);

    draw_digit(x, y_start, clock_time[3]);
    x += digit_width + digit_gap;

    draw_digit(x, y_start, clock_time[4]);
}

void myScreen::draw_clock_time(
    const std::string& clock_time,
    const std::string& color_name,
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
        color_name,
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
void myScreen::draw_buttonhint(
    char button,
    const ActionIcons::ActionIcon& action_icon,
    cont std::string& color
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