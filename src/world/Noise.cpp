#include "world/Noise.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>

namespace
{
    float Fade(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }
    float Lerp(float a, float b, float t) { return a + t * (b - a); }

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
        const int xi = (static_cast<int>(std::floor(x)) & 255);
        const int yi = (static_cast<int>(std::floor(y)) & 255);

        const float xf = x - std::floor(x);
        const float yf = y - std::floor(y);

        const float u = Fade(xf);
        const float v = Fade(yf);

        const int aa = perm[perm[xi] + yi];
        const int ab = perm[perm[xi] + yi + 1];
        const int ba = perm[perm[xi + 1] + yi];
        const int bb = perm[perm[xi + 1] + yi + 1];

        const float x1 = Lerp(Grad(aa, xf, yf), Grad(ba, xf - 1.0f, yf), u);
        const float x2 = Lerp(Grad(ab, xf, yf - 1.0f), Grad(bb, xf - 1.0f, yf - 1.0f), u);

        return Lerp(x1, x2, v);
    }
}

namespace world
{
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
                    const float nx = (static_cast<float>(x) / baseScale) * freq;
                    const float ny = (static_cast<float>(y) / baseScale) * freq;

                    sum += Perlin2D(nx, ny, perm) * amp;
                    ampSum += amp;

                    amp *= p.persistence;
                    freq *= p.lacunarity;
                }

                if (ampSum > 0.0f) sum /= ampSum;
                out[static_cast<size_t>(y) * w + x] = sum;
            }
        }

        return out;
    }

    std::vector<uint8_t> NormalizeToU8(const std::vector<float>& src)
    {
        if (src.empty()) return {};

        auto [mnIt, mxIt] = std::minmax_element(src.begin(), src.end());
        float mn = *mnIt;
        float mx = *mxIt;
        if (std::abs(mx - mn) < 1e-8f) mx = mn + 1e-8f;

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
}
