#pragma once
#include <cstdint>
#include <vector>

namespace world
{
    // Parameters controlling fBm Perlin noise generation
    struct NoiseParams
    {
        float scale = 128.0f;     // Large# → fewe, large continents / Small# → many small islands | Continents: 256 – 1024 / Archipelagos: 64 – 128 / Default 128 | scale ≈ map_width / 2 produces 1–3 major landmasses.
        int   octaves = 6;        // Number of noise layers stacked together | Continents only: 4 – 5 Continents + mountains: 6 – 7 | Default 5
        float persistence = 0.5f; // Amplitude falloff per octave | Lower = smoother terrain Higher = rough, mountainous terrain | Smooth continents: 0.45 – 0.55 Rough / jagged worlds: 0.6 – 0.7 | Default 0.5
        float lacunarity = 2.0f;  // Frequency growth per octave | Default 2.0
        uint32_t seed = 1337;
        float offsetX = 0.0f;
        float offsetY = 0.0f;
    };

    // ---------------------------------------------------------------------
    // Noise generation
    // ---------------------------------------------------------------------

    // Returns floats (roughly in [-1, 1], fBm is normalized by amplitude sum)
    std::vector<float> PerlinFbm2D(int w, int h, const NoiseParams& p);

    // ---------------------------------------------------------------------
    // Generic utilities (non-terrain-specific)
    // ---------------------------------------------------------------------

    // Normalize float field to 0..255 using auto min/max
    std::vector<uint8_t> NormalizeToU8(const std::vector<float>& src);

    // Convert grayscale to RGBA8888 bytes (size = w * h * 4)
    std::vector<uint8_t> GrayToRGBA(const std::vector<uint8_t>& gray);

    // ---------------------------------------------------------------------
    // Terrain-specific post-processing
    // ---------------------------------------------------------------------

    // Robust terrain normalization for continent generation:
    //  - clipLow / clipHigh: percentile clamps (0..1), e.g. 0.02 / 0.98 | Ignore extreme outliers in the noise distribution. Prevent “everything turns white” or “everything turns black”
    //  - SeaLevel: 0..1 (0.50 = neutral, higher => more ocean)
    //  - gamma: game curve (>1 darkens midtones, sharper coastlines)
    std::vector<uint8_t> NormalizeTerrainToU8(
        const std::vector<float>& src,
        float clipLow = 0.02f,
        float clipHigh = 0.98f,
        float SeaLevel = 0.55f, // Biases the heightmap before gamma | 0.50 → neutral | > 0.50 → more ocean | < 0.50 → more land
        float gamma = 1.45f);
}
