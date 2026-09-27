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
}
