#pragma once

#include <cstdint>

namespace Icons {

constexpr int ICON_WIDTH = 8;
constexpr int ICON_HEIGHT = 8;

struct Icon {
    const uint8_t* data;
    int width;
    int height;
};

// -----------------------------------------------------------------------------
// Asterisk
// -----------------------------------------------------------------------------

constexpr uint8_t ASTERISK_DATA[8] = {
    0b00011000,
    0b10011001,
    0b01011010,
    0b00111100,
    0b00111100,
    0b01011010,
    0b10011001,
    0b00011000
};

constexpr Icon ASTERISK = {
    ASTERISK_DATA,
    ICON_WIDTH,
    ICON_HEIGHT
};

// -----------------------------------------------------------------------------
// Heart
// -----------------------------------------------------------------------------

constexpr uint8_t HEART_DATA[8] = {
    0b00000000,
    0b01100110,
    0b11111111,
    0b11111111,
    0b01111110,
    0b00111100,
    0b00011000,
    0b00000000
};

constexpr Icon HEART = {
    HEART_DATA,
    ICON_WIDTH,
    ICON_HEIGHT
};

}