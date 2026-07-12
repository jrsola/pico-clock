#include "color.h"

const std::map<std::string, std::tuple<uint8_t, uint8_t, uint8_t>> Color::color_map = {
    {"black",       {0,0,0}},
    {"white",       {255,255,255}},
    {"red",         {255,0,0}},
    {"green",       {0,255,0}},
    {"blue",        {0,0,255}},
    {"yellow",      {255,255,0}},
    {"cyan",        {0,255,255}},
    {"magenta",     {255,0,255}},
    {"orange",      {255,165,0}},
    {"purple",      {128,0,128}},
    {"pink",        {255,192,203}},
    {"light blue",  {173,216,230}},
    {"light green", {144,238,144}},
    {"dark blue",   {0,0,139}},
    {"dark green",  {1,50,32}},
    {"light grey",  {192,192,192}},
    {"grey",        {128,128,128}},
    {"dark grey",   {32,32,32}}
};

std::tuple<uint8_t, uint8_t, uint8_t> Color::get_rgb(const std::string& color_name) {
    auto match = color_map.find(color_name);
    if (match != color_map.end()) {
        return match->second;  
    } else {
        return {255, 255, 255};
    }
}

std::string Color::get_color_name(const std::tuple<uint8_t, uint8_t, uint8_t>& rgb) {
    auto [r, g, b] = rgb;
    for (const auto& [name, color] : color_map) {
        auto [cr, cg, cb] = color;
        if (r == cr && g == cg && b == cb) {
            return name;
        }
    }
    return "";
}

uint8_t Color::get_rgb332(const std::string& color_name) {
    auto [r, g, b] = get_rgb(color_name);

    uint8_t r3 = r >> 5;  // 8 bits -> 3 bits
    uint8_t g3 = g >> 5;  // 8 bits -> 3 bits
    uint8_t b2 = b >> 6;  // 8 bits -> 2 bits

    return (r3 << 5) | (g3 << 2) | b2;
}

uint8_t Color::fade_rgb332(uint8_t color, uint8_t brightness) {
    // brightness: 0 = black, 255 = original color

    uint8_t r3 = (color >> 5) & 0x07;
    uint8_t g3 = (color >> 2) & 0x07;
    uint8_t b2 = color & 0x03;

    r3 = (r3 * brightness) / 255;
    g3 = (g3 * brightness) / 255;
    b2 = (b2 * brightness) / 255;

    return (r3 << 5) | (g3 << 2) | b2;
}