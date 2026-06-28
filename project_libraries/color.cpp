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
    {"dark green",  {1,50,32}}
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