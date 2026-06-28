#include "my_led.h"

myLED::myLED() : led(PicoDisplay2::LED_R, PicoDisplay2::LED_G, PicoDisplay2::LED_B) {
    this->led_color = {0, 0, 0};
    this->set_rgb(led_color);
}

void myLED::set_state(bool state) {
    if (state) {
        set_brightness(this->led_brightness);
    } else {
        set_brightness(0);
    }
}

bool myLED::get_state() {
    if (this->led_brightness) {
        return true;
    }
    return false;
}

void myLED::set_brightness(uint8_t value) {
    if (value <= 255) {
        this->led_brightness = value;
        led.set_brightness(value);
    }
}

uint8_t myLED::get_brightness() {
    return this->led_brightness;
}

void myLED::set_rgb(const std::string& color_name) {
    auto [r, g, b] = Color::get_rgb(color_name);
    this->set_rgb(r, g, b);
}

void myLED::set_rgb(std::tuple<uint8_t, uint8_t, uint8_t> rgb_tuple) {
    auto [r, g, b] = rgb_tuple;
    this->set_rgb(r, g, b);
}

void myLED::set_rgb(uint8_t r, uint8_t g, uint8_t b) {
    this->led_color = {r, g, b};
    led.set_rgb(r, g, b);
}

std::tuple<uint8_t, uint8_t, uint8_t> myLED::get_rgb() {
    return this->led_color;
}

void myLED::set_blink_on() {
    for (uint8_t i = 0; i <= this->led_brightness; i++) {
        led.set_brightness(i);
        sleep_ms(2);
    }
    this->n_blinks--;
}

void myLED::set_blink_off() {
    for (uint8_t i = this->led_brightness; i > 0; i--) {
        led.set_brightness(i);
        sleep_ms(2);
    }
    this->n_blinks--;
}

int myLED::new_blink(uint8_t blinks, uint16_t delay_ms, const std::string& blink_color) {
    if (is_blinking) {
        return -1;
    }
    this->blink_precolor = this->led_color;
    set_rgb(blink_color);
    is_blinking = true;
    n_blinks = blinks * 2;
    blink_delay_ms = delay_ms;
    last_blink_time = millis();
    return 0;
}

int myLED::blink_update() {
    if (!is_blinking) {
        return 0;
    }

    if (millis() - this->last_blink_time >= this->blink_delay_ms) {
        if (this->get_state()) {
            this->set_blink_off();
        } else {
            this->set_blink_on();
        }
        this->last_blink_time = millis();
        if (this->n_blinks == 0) {
            this->is_blinking = false;
            set_rgb(this->blink_precolor);
            set_brightness(this->led_brightness);
        }
    }
    return n_blinks;
}