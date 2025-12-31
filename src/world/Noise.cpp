// Noise.cpp
#include "world/Noise.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>

namespace
{
    // ---------------------------------------------------------------------
    // Core Perlin helpers
    // ---------------------------------------------------------------------

    float Fade(float t)
    {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    float Lerp(float a, float b, float t)
    {
        return a + t * (b - a);
    }

    float Grad(int hash, float x, float y)
    {
        const int h = hash & 3;
        const float u = (h < 2) ? x : y;
        const float v = (h < 2) ? y : x;
        return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
    }

    std::array<int, 512> BuildPerm(uint32_t seed)
    {
        std::array<int, 256> p{};
        std::iota(p.begin(), p.end(), 0);

        std::mt19937 rng(seed);
        std::shuffle(p.begin(), p.end(), rng);

        std::array<int, 512> perm{};
        for (int i = 0; i < 512; ++i)
            perm[i] = p[i & 255];

        return perm;
    }

    float Perlin2D(float x, float y, const std::array<int, 512>& perm)
    {
        const int xi = static_cast<int>(std::floor(x)) & 255;
        const int yi = static_cast<int>(std::floor(y)) & 255;

        const float xf = x - std::floor(x);
        const float yf = y - std::floor(y);

        const float u = Fade(xf);
        const float v = Fade(yf);

        const int aa = perm[perm[xi] + yi];
        const int ab = perm[perm[xi] + yi + 1];
        const int ba = perm[perm[xi + 1] + yi];
        const int bb = perm[perm[xi + 1] + yi + 1];

        const float x1 = Lerp(
            Grad(aa, xf, yf),
            Grad(ba, xf - 1.0f, yf),
            u
        );

        const float x2 = Lerp(
            Grad(ab, xf, yf - 1.0f),
            Grad(bb, xf - 1.0f, yf - 1.0f),
            u
        );

        return Lerp(x1, x2, v);
    }
}

namespace world
{
    // ---------------------------------------------------------------------
    // fBm Perlin generation
    // ---------------------------------------------------------------------

    std::vector<float> PerlinFbm2D(int w, int h, const NoiseParams& p)
    {
        std::vector<float> out(static_cast<size_t>(w) * static_cast<size_t>(h), 0.0f);
        const auto perm = BuildPerm(p.seed);

        const float baseScale = (p.scale <= 0.0001f) ? 0.0001f : p.scale;

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                float amp = 1.0f;
                float freq = 1.0f;
                float sum = 0.0f;
                float ampSum = 0.0f;

                for (int o = 0; o < p.octaves; ++o)
                {
                    const float nx = ((static_cast<float>(x) + p.offsetX) / baseScale) * freq;
                    const float ny = ((static_cast<float>(y) + p.offsetY) / baseScale) * freq;

                    sum += Perlin2D(nx, ny, perm) * amp;
                    ampSum += amp;

                    amp *= p.persistence;
                    freq *= p.lacunarity;
                }

                if (ampSum > 0.0f)
                    sum /= ampSum;

                out[static_cast<size_t>(y) * w + x] = sum;
            }
        }

        return out;
    }

    // ---------------------------------------------------------------------
    // Generic normalization utilities
    // ---------------------------------------------------------------------

    std::vector<uint8_t> NormalizeToU8(const std::vector<float>& src)
    {
        if (src.empty()) return {};

        auto [mnIt, mxIt] = std::minmax_element(src.begin(), src.end());
        float mn = *mnIt;
        float mx = *mxIt;
        if (std::abs(mx - mn) < 1e-8f)
            mx = mn + 1e-8f;

        std::vector<uint8_t> out(src.size());

        for (size_t i = 0; i < src.size(); ++i)
        {
            float t = (src[i] - mn) / (mx - mn);
            t = std::clamp(t, 0.0f, 1.0f);
            out[i] = static_cast<uint8_t>(t * 255.0f + 0.5f);
        }

        return out;
    }

    std::vector<uint8_t> GrayToRGBA(const std::vector<uint8_t>& gray)
    {
        std::vector<uint8_t> rgba(gray.size() * 4);

        for (size_t i = 0; i < gray.size(); ++i)
        {
            const uint8_t g = gray[i];
            rgba[i * 4 + 0] = g;
            rgba[i * 4 + 1] = g;
            rgba[i * 4 + 2] = g;
            rgba[i * 4 + 3] = 255;
        }

        return rgba;
    }

    // ---------------------------------------------------------------------
    // Terrain-specific normalization (continents)
    // ---------------------------------------------------------------------

    static float Clamp01(float x)
    {
        return std::clamp(x, 0.0f, 1.0f);
    }

    static float Percentile(std::vector<float> data, float p01)
    {
        if (data.empty()) return 0.0f;
        p01 = std::clamp(p01, 0.0f, 1.0f);

        const size_t n = data.size();
        const size_t k = static_cast<size_t>(std::round(p01 * float(n - 1)));

        std::nth_element(data.begin(), data.begin() + k, data.end());
        return data[k];
    }

    std::vector<uint8_t> NormalizeTerrainToU8(
        const std::vector<float>& src,
        float clipLow,
        float clipHigh,
        float SeaLevel,
        float gamma)
    {
        if (src.empty()) return {};

        const float lo = Percentile(src, clipLow);
        const float hi = Percentile(src, clipHigh);
        float denom = hi - lo;
        if (std::abs(denom) < 1e-8f)
            denom = 1e-8f;

        // SeaLevel: 0.50 = neutral
        const float seaBias = SeaLevel - 0.5f;

        std::vector<uint8_t> out(src.size());

        for (size_t i = 0; i < src.size(); ++i)
        {
            // 1) Robust normalization
            float t = (src[i] - lo) / denom;
            t = Clamp01(t);

            // 2) Sea level bias (controls land/ocean ratio)
            t = Clamp01(t - seaBias);

            // 3) Game curve (gamma)
            if (gamma > 0.0001f)
                t = std::pow(t, gamma);

            out[i] = static_cast<uint8_t>(t * 255.0f + 0.5f);
        }

        return out;
    }
}
