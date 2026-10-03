#pragma once

#include <string>
#include <ctime>
#include <string_view>
#include <optional>

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
        // this will prevent text from wrapping on screen
        static constexpr int NO_WRAP = 10000;    
        
        // class attribute to save the backlight level 
        uint8_t backlight = 255;

        Colors::Color pen_color = Colors::WHITE;
        Colors::Color background_color = Colors::BLACK;

        // screen controller chipset
        ST7789 st7789;
        
        std::string last_clock_time = "";

        //draw one button hint on screen
        void draw_buttonhint(char button, const ActionIcons::ActionIcon& action_icon);

    public:
        // constructor
        PicoScreen();
        
        // get screen dimensions
        uint16_t get_width();
        uint16_t get_height();
        
        // send framebuffer to physical display, so it gets updated
        void update();
        
        // call the chipset method to set the screen backlight level
        void set_backlight(uint8_t backlight);

        // retrieve the current screen backlight level 
        uint8_t get_backlight();
        
        // set the pen color using Pimononi's library and save its value in a class attribute
        void set_pen(const Colors::Color& color);
        
        // retrieve the current pen color
        Colors::Color get_pen();

        // call the standard rectangle method wihtout having to use a Rect object
        void rectangle(int x, int y, int width, int height);
        
        // clear the screen, i.e. fill it all with a background color
        // optionally we can add a fade effect (default is no fading)
        // we can update the screen optionally (default is update)
        void clear(const Colors::Color& color, int fade_steps = 0, bool upd = true);
        
        // write a text at the x/y coordinates with a certain color and scale
        // defaults are: no text, white color and scale = 2
        void writexy(int x, int y, const std::string_view &t = "", const Colors::Color& color = Colors::WHITE, int scale = 2, bool upd= true);
        
        // draw a fading bootup logo using the image in logo_rgb332.h
        // optionally, can show a text under the logo (default is none), 
        // fading speed can be contolled (default is 100 ms for each fading step)
        void draw_logo(const std::string& title = "", int delay = 100);
        
        // show the booting progress in the lower part of the screen.
        // shows a centered message and/or a progress bar (both optional)
        // accepts the text and a color for the text, and a percentage for the progress bar
        // -1 means delete the progress bar, no progress bar percentage means leave it as is
        void boot_progress(std::string_view message="", const Colors::Color& = Colors::WHITE, std::optional<int> progress = std::nullopt);
        
         // draw a big digital clock to be used as a screensaver
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
