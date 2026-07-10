#ifndef COLOR_H
#define COLOR_H

#include <string>
#include <map>
#include <tuple>
#include <cstdint>

class Color {
private:
    static const std::map<std::string, std::tuple<uint8_t, uint8_t, uint8_t>> color_map;

public:
    static std::tuple<uint8_t, uint8_t, uint8_t> get_rgb(const std::string& color_name);
    static std::string get_color_name(const std::tuple<uint8_t, uint8_t, uint8_t>& rgb);
    static uint8_t get_rgb332(const std::string& color_name);
    static uint8_t fade_rgb332(uint8_t color, uint8_t brightness);
};

#endif