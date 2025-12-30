#pragma once
#include <cstdint>

struct Color
{
    uint8_t r=0, g=0, b=0, a=255;

    static Color RGB(uint8_t rr, uint8_t gg, uint8_t bb) { return {rr, gg, bb, 255}; }
    static Color RGBA(uint8_t rr, uint8_t gg, uint8_t bb, uint8_t aa) { return {rr, gg, bb, aa}; }
};
