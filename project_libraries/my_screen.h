#pragma once

#include <string>
#include <vector>
#include <ctime>

#include "pico_display_2.hpp"
#include "pico_graphics.hpp"
#include "st7789.hpp"

#include "color.h"
#include "logo_rgb332.h"
#include "icons.h"

using namespace pimoroni;

// our class is going to piggyback on the Pimoroni one for this screen
// that works with RGB332
class PicoScreen : public PicoGraphics_PenRGB332 {
    private:
        uint8_t backlight = 255;

        Colors::Color pen_color = Colors::WHITE;
        Colors::Color background_color = Colors::BLACK;

        ST7789 st7789;
        std::vector<uint8_t> frame_buffer;
        
        int textx, texty, twidth;
        int progress_segments = 0;
        std::string last_clock_time = "";

        //draw one button hint on screen
        void draw_buttonhint(char button, const ActionIcons::ActionIcon& action_icon);

    public:
        // constructor
        PicoScreen();
        
        // get screen dimensions
        uint16_t get_width();
        uint16_t get_height();
        
        // get and set screen brightness
        void set_brightness(uint8_t backlight);
        uint8_t get_brightness();
        
        // get and set pen color
        void set_pen(const Colors::Color& color);
        Colors::Color get_pen();

        void clear(const Colors::Color& color, int fade_steps = 0, bool upd = true);
        
        void update();
        
        void writexy(int x, int y, const std::string_view &t = "", const Colors::Color& color = Colors::WHITE, int scale = 2);
        
        void draw_logo(const std::string& title = "", const int delay = 100);
        
        void progress_bar(int segments = 13);
        
        void show_boot_message(std::string_view boot_msg = "", const Colors::Color& color = Colors::YELLOW);
        
        void draw_clock_time(int x, int y, const std::string& clock_time, const Colors::Color& color = Colors::YELLOW, int size = 6, bool force_redraw = false);
        void draw_clock_time(const std::string& clock_time, const Colors::Color& color = Colors::YELLOW, int size = 6, bool force_redraw = false);
        
        // draw the 4 button hints (a/b/x/y)
        void draw_buttonhints(
            const ActionIcons::ActionIcon& button_a,
            const ActionIcons::ActionIcon& button_b,
            const ActionIcons::ActionIcon& button_x,
            const ActionIcons::ActionIcon& button_y
        );
};
