#pragma once
#include <cstdint>

namespace cfg
{
    // Window
    constexpr int WindowWidth  = 1280;
    constexpr int WindowHeight = 720;

    // Rendering
    constexpr int TileSizePx = 16;           // tiles are 16x16 pixels
    constexpr int FontGlyphPx = 16;          // font cell size (16x16)
    constexpr const char* FontAtlasPath = "assets/fonts/font16x16.bmp";

    // World
    constexpr int WorldW = 80;
    constexpr int WorldH = 45;
}
