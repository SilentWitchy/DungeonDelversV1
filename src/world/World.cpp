#include "world/World.h"
#include "core/Config.h"
#include "gfx/Renderer.h"
#include "gfx/Font.h"
#include "gfx/Color.h"
#include <algorithm>
#include <string>
#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
#include <random>
#include <vector>

World::World()
    : m_width(cfg::WorldW)
    , m_height(cfg::WorldH)
    , m_tiles(m_width * m_height)
{
    WorldGenSettings s; // defaults to "middle" options
    Generate(s);
}

World::World(const WorldGenSettings& settings)
    : m_width(cfg::WorldW)
    , m_height(cfg::WorldH)
    , m_tiles(m_width * m_height)
{
    Generate(settings);
}

World::World(const WorldGenSettings& settings, const WorldHistoryPackage& history)
    : m_width(cfg::WorldW)
    , m_height(cfg::WorldH)
    , m_tiles(m_width * m_height)
{
    Generate(settings);
    ApplyHistorySpawns(history);
}

static uint32_t XorShift32(uint32_t& x)
{
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

static int RandRange(uint32_t& st, int lo, int hi)
{
    if (hi <= lo) return lo;
    const uint32_t r = XorShift32(st);
    const int span = (hi - lo + 1);
    return lo + (int)(r % (uint32_t)span);
}

class Perlin2D
{
public:
    explicit Perlin2D(uint32_t seed)
    {
        std::iota(m_perm.begin(), m_perm.begin() + 256, 0);
        std::mt19937 rng(seed);
        std::shuffle(m_perm.begin(), m_perm.begin() + 256, rng);
        for (size_t i = 0; i < 256; ++i)
            m_perm[256 + i] = m_perm[i];
    }

    float Noise(float x, float y) const
    {
        const int xi = static_cast<int>(std::floor(x)) & 255;
        const int yi = static_cast<int>(std::floor(y)) & 255;
        const float xf = x - std::floor(x);
        const float yf = y - std::floor(y);

        const float u = Fade(xf);
        const float v = Fade(yf);

        const int aa = m_perm[m_perm[xi] + yi];
        const int ab = m_perm[m_perm[xi] + yi + 1];
        const int ba = m_perm[m_perm[xi + 1] + yi];
        const int bb = m_perm[m_perm[xi + 1] + yi + 1];

        const float x1 = Lerp(Grad(aa, xf, yf), Grad(ba, xf - 1, yf), u);
        const float x2 = Lerp(Grad(ab, xf, yf - 1), Grad(bb, xf - 1, yf - 1), u);

        return (Lerp(x1, x2, v) + 1.0f) * 0.5f; // normalize to 0..1
    }

private:
    static float Fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
    static float Lerp(float a, float b, float t) { return a + t * (b - a); }
    static float Grad(int hash, float x, float y)
    {
        switch (hash & 0x3)
        {
        case 0: return  x + y;
        case 1: return -x + y;
        case 2: return  x - y;
        default:return -x - y;
        }
    }

    std::array<int, 512> m_perm{};
};

static float OctaveNoise(const Perlin2D& p, float x, float y, int octaves, float persistence)
{
    float total = 0.0f;
    float maxValue = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;

    for (int i = 0; i < octaves; ++i)
    {
        total += p.Noise(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }

    if (maxValue <= 0.0f) return 0.0f;
    return total / maxValue;
}

static TileType BiomeFromValues(float elevation, float temperature, float moisture)
{
    const float seaLevel = 0.38f;
    const float coastBand = 0.05f;
    if (elevation < seaLevel)
        return TileType::Ocean;
    if (elevation < seaLevel + coastBand)
        return TileType::Coast;

    if (elevation > 0.82f)
        return TileType::Mountain;
    if (elevation > 0.68f)
        return TileType::Hill;

    if (temperature < 0.25f)
        return TileType::Tundra;

    if (moisture < 0.25f)
        return (temperature > 0.6f) ? TileType::Desert : TileType::Plains;

    if (moisture > 0.7f && temperature > 0.6f)
        return TileType::Jungle;

    if (moisture > 0.5f)
        return TileType::Forest;

    return TileType::Plains;
}

static bool IsWater(TileType t)
{
    return t == TileType::Ocean || t == TileType::Lake || t == TileType::River;
}

static float Clamp01(float v)
{
    return std::max(0.0f, std::min(1.0f, v));
}

void World::ApplyHistorySpawns(const WorldHistoryPackage& history)
{
    uint32_t rng = (history.seed ? history.seed : 1u) ^ 0xA341316Cu;

    auto tryPlace = [&](TileType t)
        {
            for (int attempt = 0; attempt < 500; ++attempt)
            {
                const int x = RandRange(rng, 1, m_width - 2);
                const int y = RandRange(rng, 1, m_height - 2);
                Tile& tile = m_tiles[y * m_width + x];

                if (!IsWater(tile.type))
                {
                    tile.type = t;
                    return true;
                }
            }
            return false;
        };

    const int maxCities = std::min(6, (int)history.citiesToPlace.size());
    const int maxRuins = std::min(6, (int)history.ruinsToPlace.size());
    const int maxArtifacts = std::min(3, (int)history.artifactsToSeed.size());

    for (int i = 0; i < maxCities; ++i)    tryPlace(TileType::City);
    for (int i = 0; i < maxRuins; ++i)     tryPlace(TileType::Ruin);
    for (int i = 0; i < maxArtifacts; ++i) tryPlace(TileType::Artifact);

    const int dungeonCount = std::min(2, (int)history.ruinsToPlace.size() / 3);
    for (int i = 0; i < dungeonCount; ++i) tryPlace(TileType::DungeonSite);
}

void World::Generate(const WorldGenSettings& settings)
{
    const uint32_t seedBase = 0xBEEF1234u + (uint32_t)(settings.worldSize * 133) + (uint32_t)(settings.worldVolatility * 71);
    Perlin2D elevationNoise(seedBase);
    Perlin2D temperatureNoise(seedBase ^ 0x9E3779B9u);
    Perlin2D moistureNoise(seedBase ^ 0x85EBCA6Bu);
    Perlin2D mineralNoise(seedBase ^ 0xC2B2AE35u);
    Perlin2D vegetationNoise(seedBase ^ 0x27D4EB2Fu);

    const float sizeScale = 0.7f + (float)settings.worldSize * 0.15f;

    for (int y = 0; y < cfg::WorldH; ++y)
    {
        for (int x = 0; x < m_width; ++x)
        {
            const float nx = (float)x / (float)cfg::WorldW;
            const float ny = (float)y / (float)cfg::WorldH;

            float elevation = OctaveNoise(elevationNoise, nx * sizeScale, ny * sizeScale, 5, 0.55f);
            float ridges = OctaveNoise(elevationNoise, nx * sizeScale * 0.5f, ny * sizeScale * 0.5f, 3, 0.65f);
            elevation = Clamp01(elevation * 0.6f + ridges * 0.4f);

            const float latitude = 1.0f - std::abs(ny * 2.0f - 1.0f);
            float temperature = Clamp01((latitude * 0.7f) + (OctaveNoise(temperatureNoise, nx * 1.2f, ny * 1.2f, 4, 0.6f) * 0.6f));

            float moisture = Clamp01(OctaveNoise(moistureNoise, nx * 1.8f, ny * 1.8f, 4, 0.55f));
            float mineral = Clamp01(OctaveNoise(mineralNoise, nx * 2.2f, ny * 2.2f, 3, 0.6f));
            float vegetation = Clamp01(OctaveNoise(vegetationNoise, nx * 1.6f, ny * 1.6f, 4, 0.6f));

            Tile& t = m_tiles[y * cfg::WorldW + x];
            t.elevation = elevation;
            t.temperature = temperature;
            t.moisture = moisture;
            t.mineralRichness = mineral;
            t.vegetationDensity = vegetation;
            t.hasRiver = false;
            t.isLake = false;
            t.isStartingPoint = false;
            t.type = BiomeFromValues(elevation, temperature, moisture);
        }
    }

    // Lake pass: deepen low elevation and wet spots into lakes
    for (int y = 1; y < cfg::WorldH - 1; ++y)
    {
        for (int x = 1; x < cfg::WorldW - 1; ++x)
        {
            Tile& t = m_tiles[y * cfg::WorldW + x];
            if (!IsWater(t.type) && t.elevation < 0.42f && t.moisture > 0.55f)
            {
                t.type = TileType::Lake;
                t.isLake = true;
            }
        }
    }

    // River pass: march downhill until hitting water
    uint32_t rng = seedBase ^ 0xA341316Cu;
    const int riverCount = 2 + settings.worldSize;
    const int maxRiverLength = cfg::WorldW + cfg::WorldH;
    const int dirs[8][2] = { {1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1},{0,-1},{1,-1} };

    for (int r = 0; r < riverCount; ++r)
    {
        int rx = RandRange(rng, 4, cfg::WorldW - 5);
        int ry = RandRange(rng, 4, cfg::WorldH - 5);

        // find a high starting point
        for (int attempt = 0; attempt < 50; ++attempt)
        {
            const int tx = RandRange(rng, 4, cfg::WorldW - 5);
            const int ty = RandRange(rng, 4, cfg::WorldH - 5);
            if (m_tiles[ty * cfg::WorldW + tx].elevation > m_tiles[ry * cfg::WorldW + rx].elevation)
            {
                rx = tx; ry = ty;
            }
        }

        for (int step = 0; step < maxRiverLength; ++step)
        {
            Tile& t = m_tiles[ry * cfg::WorldW + rx];
            if (IsWater(t.type) && !(t.hasRiver))
                break;

            t.type = TileType::River;
            t.hasRiver = true;

            // choose neighbor with lowest elevation
            float bestElevation = t.elevation;
            int bestDx = 0, bestDy = 0;
            for (auto& d : dirs)
            {
                const int nx = rx + d[0];
                const int ny = ry + d[1];
                if (nx <= 0 || ny <= 0 || nx >= cfg::WorldW - 1 || ny >= cfg::WorldH - 1)
                    continue;
                const Tile& nt = m_tiles[ny * cfg::WorldW + nx];
                if (nt.elevation <= bestElevation)
                {
                    bestElevation = nt.elevation;
                    bestDx = d[0];
                    bestDy = d[1];
                }
            }

            if (bestDx == 0 && bestDy == 0)
                break;

            rx += bestDx;
            ry += bestDy;

            if (IsWater(m_tiles[ry * cfg::WorldW + rx].type))
                break;
        }
    }

    // Re-evaluate biomes to respect rivers and lakes visually
    for (int y = 0; y < cfg::WorldH; ++y)
    {
        for (int x = 0; x < cfg::WorldW; ++x)
        {
            Tile& t = m_tiles[y * cfg::WorldW + x];
            if (t.hasRiver)
                t.type = TileType::River;
            else if (t.isLake)
                t.type = TileType::Lake;
            else
                t.type = BiomeFromValues(t.elevation, t.temperature, t.moisture);
        }
    }

    // Pick a starting position on hospitable land
    m_startPos = { cfg::WorldW / 2, cfg::WorldH / 2 };
    float bestScore = -1.0f;
    for (int y = 0; y < cfg::WorldH; ++y)
    {
        for (int x = 0; x < cfg::WorldW; ++x)
        {
            Tile& t = m_tiles[y * cfg::WorldW + x];
            if (IsWater(t.type))
                continue;

            const float tempComfort = 1.0f - std::abs(t.temperature - 0.55f);
            const float score = (t.vegetationDensity * 0.35f) + (t.mineralRichness * 0.35f) + (tempComfort * 0.2f) + (t.elevation * 0.1f);
            if (score > bestScore)
            {
                bestScore = score;
                m_startPos = { x, y };
            }
        }
    }

    Tile& startTile = m_tiles[m_startPos.second * cfg::WorldW + m_startPos.first];
    startTile.isStartingPoint = true;
    startTile.type = TileType::Core;
}

void World::Tick()
{
    // placeholder: later this advances simulation
}

void World::Render(Renderer& r, Font& font, int camX, int camY)
{
    const int tilesAcross = cfg::WindowWidth / cfg::TileSizePx;
    const int tilesDown   = cfg::WindowHeight / cfg::TileSizePx;

    for (int sy = 0; sy < tilesDown; ++sy)
    {
        for (int sx = 0; sx < tilesAcross; ++sx)
        {
            const int wx = camX + sx;
            const int wy = camY + sy;
            if (wx < 0 || wy < 0 || wx >= m_width || wy >= m_height)
                continue;

            const Tile& t = m_tiles[wy * m_width + wx];
            const int px = sx * cfg::TileSizePx;
            const int py = sy * cfg::TileSizePx;

            Color base = TileColor(t);
            const float mineralBoost = t.mineralRichness * 0.25f;
            const float vegBoost = t.vegetationDensity * 0.15f;
            base = Color(
                std::min(255, (int)(base.r + base.r * (mineralBoost + vegBoost))),
                std::min(255, (int)(base.g + base.g * (vegBoost))),
                std::min(255, (int)(base.b + base.b * (mineralBoost * 0.5f)))
            );

            r.FillRect(px, py, cfg::TileSizePx, cfg::TileSizePx, base);

            char glyph = t.isStartingPoint ? 'P' : TileGlyph(t);
            char s[2] = { glyph, 0 };
            font.DrawText(r, px, py, s);
        }
    }

    font.DrawText(r, 8, cfg::WindowHeight - 24, "World map: elevation, rivers, lakes, and biomes");
}

void World::RenderFullMap(Renderer& r, Font& font, int camX, int camY)
{
    (void)camX;
    (void)camY;

    const int tilePx = std::max(1, std::min(cfg::WindowWidth / cfg::WorldW, cfg::WindowHeight / cfg::WorldH));
    const int mapPxW = cfg::WorldW * tilePx;
    const int mapPxH = cfg::WorldH * tilePx;
    const int offsetX = (cfg::WindowWidth - mapPxW) / 2;
    const int offsetY = (cfg::WindowHeight - mapPxH) / 2;

    for (int y = 0; y < cfg::WorldH; ++y)
    {
        for (int x = 0; x < cfg::WorldW; ++x)
        {
            const Tile& t = m_tiles[y * cfg::WorldW + x];
            const int px = offsetX + x * tilePx;
            const int py = offsetY + y * tilePx;

            r.FillRect(px, py, tilePx, tilePx, TileColor(t));

            if (tilePx >= font.GlyphW())
            {
                char s[2] = { TileGlyph(t), 0 };
                font.DrawText(r, px, py, s);
            }
        }
    }

    r.DrawRect(offsetX - 2, offsetY - 2, mapPxW + 4, mapPxH + 4, Color::RGB(200, 200, 200));

    const int viewTilesW = cfg::WindowWidth / cfg::TileSizePx;
    const int viewTilesH = cfg::WindowHeight / cfg::TileSizePx;
    const int viewPxW = viewTilesW * tilePx;
    const int viewPxH = viewTilesH * tilePx;
    const int viewX = offsetX + camX * tilePx;
    const int viewY = offsetY + camY * tilePx;
    r.DrawRect(viewX, viewY, viewPxW, viewPxH, Color::RGB(255, 255, 255));

    font.DrawText(r, 12, 12, "Full map view (M to close)");
}
