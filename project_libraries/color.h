#pragma once

#include <cstdint>

namespace Colors
{
    // define a color based on its three components
    struct Color {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

    // colors
    constexpr Color BLACK       = {0,0,0};
    constexpr Color WHITE       = {255,255,255};
    constexpr Color RED         = {255,0,0};
    constexpr Color GREEN       = {0,255,0};
    constexpr Color BLUE        = {0,0,255};
    constexpr Color YELLOW      = {255,255,0};
    constexpr Color CYAN        = {0,255,255};
    constexpr Color MAGENTA     = {255,0,255};
    constexpr Color ORANGE      = {255,165,0};
    constexpr Color PURPLE      = {128,0,128};
    constexpr Color PINK        = {255,192,203};
    constexpr Color LIGHT_BLUE  = {173,216,230};
    constexpr Color LIGHT_GREEN = {144,238,144};
    constexpr Color DARK BLUE   = {0,0,64};
    constexpr Color DARK_GREEN  = {1,50,32};
    constexpr Color LIGHT_GREY  = {192,192,192};
    constexpr Color GREY        = {128,128,128};
    constexpr Color DARK_GREY   = {32,32,32};

    // Pimoroni screen works with rgb332
    // convert rgb888 (24 bits) into rgb332 (8 bits) 
    // rgb332: 3 bits for red, 3 bits for green, 2 bits for blue
    constexpr uint8_t to_rgb332(const Color& color) {
        // each component (.r/.g/.b) has 8 bits
        // we shift the values so we only consider 3 bits for r/g and 2 for b 
        uint8_t r3 = color.r >> 5; // shift 5 bits
        uint8_t g3 = color.g >> 5; // shift 5 bits 
        uint8_t b2 = color.b >> 6; // shift 6 bits

        // return an 8 bit value by overlaying the individual values
        // for each component (shifting them to the right positions)
        // 76543210
        // RRRGGGBB  
        return (r3 << 5) | (g3 << 2) | b2;
    }

    // fade an rgb332 color to black
    // brightness: 0 = black, 255 = original color
    constexpr uint8_t fade_rgb332(uint8_t color, uint8_t brightness){
        // divide the 8 bit rgb color value into each component
        uint8_t r3 = (color >> 5); // shift 5 bits 
        uint8_t g3 = (color >> 2) & 0b00000111; // shift 2 bits and discard other bits
        uint8_t b2 = color & 0b00000011; // just keep 2 last bits for blue

        // multiply each component by brightness / 255
        r3 = (r3 * brightness) / 255;
        g3 = (g3 * brightness) / 255;
        b2 = (b2 * brightness) / 255;

        // pack each component value back into an 8 bit value 
        return (r3 << 5) | (g3 << 2) | b2;
    }

    // this is a special case of fading, but towards another color
    constexpr uint8_t transition_to_rgb332(uint8_t from, uint8_t to) {
    uint8_t from_r = from >> 5;
    uint8_t from_g = (from >> 2) & 0b00000111;
    uint8_t from_b = from & 0b00000011;

    uint8_t to_r = to >> 5;
    uint8_t to_g = (to >> 2) & 0b00000111;
    uint8_t to_b = to & 0b00000011;

    if (from_r < to_r) from_r++;
    else if (from_r > to_r) from_r--;

    if (from_g < to_g) from_g++;
    else if (from_g > to_g) from_g--;

    if (from_b < to_b) from_b++;
    else if (from_b > to_b) from_b--;

    return (from_r << 5) | (from_g << 2) | from_b;
}
}
