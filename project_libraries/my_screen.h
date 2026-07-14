#include <string>
#include <tuple>
#include <vector>
#include <ctime>
#include "pico_display_2.hpp"
#include "pico_graphics.hpp"
#include "st7789.hpp"
#include "color.h"
#include "logo_rgb332.h"
#include "icons.h"

using namespace pimoroni;

class myScreen {
private:
    const uint16_t WIDTH = 320;
    const uint16_t HEIGHT = 240;
    uint8_t backlight = 255;
    std::tuple<uint8_t, uint8_t, uint8_t> pen_color;
    ST7789 st7789;
    std::vector<uint8_t> frame_buffer;
    pimoroni::PicoGraphics_PenRGB332 screen;
    std::string background_color = "black";
    int textx, texty, twidth;
    int progress_segments = 0;
    std::string last_clock_time = "";

public:
    myScreen();
    uint16_t get_width();
    uint16_t get_height();
    void set_brightness(uint8_t backlight);
    uint8_t get_brightness();
    void set_pen(const std::string& color_name);
    void set_pen(std::tuple<uint8_t, uint8_t, uint8_t> rgb_tuple);
    void set_pen(uint8_t r, uint8_t g, uint8_t b);
    std::tuple<uint8_t, uint8_t, uint8_t> get_pen();
    void pixel(const Point &p);
    void clear(const std::string& color_name = "black", int fade_steps = 0, bool upd = true);
    void update();
    void rectangle(int x, int y, int width, int height);
    void writeln(const std::string_view &t = "", const std::string& color_name = "");
    void writexy(int x, int y, const std::string_view &t = "", const std::string& color_name = "", int scale = 2);
    void draw_logo(const std::string& title = "", const int steps = 15, const int delay = 100);
    void progress_bar(int segments = -1);
    void show_boot_message(std::string_view boot_msg = "", const std::string& color_name = "white");
    void draw_clock_time(int x, int y, const std::string& clock_time, const std::string& color_name = "yellow", int size = 6);
    void draw_clock_time(const std::string& clock_time, const std::string& color_name = "yellow", int size = 6);
    void draw_buttonhint (int corner, const std::string& color_name, const Icons::Icon& icon);
};
