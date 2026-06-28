#include <string>
#include <tuple>
#include "pico/stdlib.h"
#include "rgbled.hpp"
#include "color.h"
#include "pico_display_2.hpp"

using namespace pimoroni;

class myLED {
private:
    pimoroni::RGBLED led;
    bool led_state = false;
    uint8_t led_brightness = 155;
    std::tuple<uint8_t, uint8_t, uint8_t> led_color;

    bool is_blinking = false;
    std::tuple<uint8_t, uint8_t, uint8_t> blink_precolor;
    uint8_t n_blinks = 0;
    uint16_t blink_delay_ms = 0;
    uint32_t last_blink_time = 0;

public:
    myLED();
    void set_state(bool state);
    bool get_state();
    void set_brightness(uint8_t value);
    uint8_t get_brightness();
    void set_rgb(const std::string& color_name);
    void set_rgb(std::tuple<uint8_t, uint8_t, uint8_t> rgb_tuple);
    void set_rgb(uint8_t r, uint8_t g, uint8_t b);
    std::tuple<uint8_t, uint8_t, uint8_t> get_rgb();
    void set_blink_on();
    void set_blink_off();
    int new_blink(uint8_t blinks, uint16_t delay_ms, const std::string& blink_color);
    int blink_update();
};