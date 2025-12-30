#pragma once
#include <cstdint>
#include <vector>

namespace world
{
    struct NoiseParams
    {
        float scale = 128.0f;     // bigger = larger features
        int   octaves = 5;
        float persistence = 0.5f; // amplitude falloff per octave
        float lacunarity = 2.0f;  // frequency growth per octave
        uint32_t seed = 1337;
    };

    // Returns floats (roughly in [-1,1], but fBm can exceed a bit before normalization)
    std::vector<float> PerlinFbm2D(int w, int h, const NoiseParams& p);

    // Normalize float field to 0..255 (auto-min/max)
    std::vector<uint8_t> NormalizeToU8(const std::vector<float>& src);

    // Convert grayscale to RGBA8888 bytes (size = w*h*4)
    std::vector<uint8_t> GrayToRGBA(const std::vector<uint8_t>& gray);
}
