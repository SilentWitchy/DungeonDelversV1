#include "ui/Ui.h"
#include "gfx/Font.h"
#include "gfx/Renderer.h"
#include "gfx/Color.h"
#include "core/Config.h"
#include "world/Noise.h"

#include <cstdint>
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace
{
    Color BurntGold() { return Color::RGB(196, 146, 64); }
    Color Ember()     { return Color::RGB(122, 82, 44); }
    Color DeepNight() { return Color::RGB(8, 10, 18); }

    const int WORLD_SIZE_TO_RESOLUTION[5] = { 17, 33, 65, 129, 257 };

    uint32_t PreviewSeed(const int wgChoice[7])
    {
        uint32_t seed = 0xA341316C; // LCG-style mix to keep preview deterministic per setting
        for (int i = 0; i < 7; ++i)
            seed = seed * 1664525u + 1013904223u + static_cast<uint32_t>(wgChoice[i] + 1);
        return seed;
    }

    int WorldResolution(int worldSizeIndex)
    {
        const int idx = std::clamp(worldSizeIndex, 0, 4);
        return WORLD_SIZE_TO_RESOLUTION[idx];
    }

    void DrawCelestialBackdrop(Renderer& r)
    {
        r.FillRect(0, 0, cfg::WindowWidth, cfg::WindowHeight, DeepNight());

        // Subtle horizontal bands
        for (int band = 0; band < 7; ++band)
        {
            const int y = band * (cfg::WindowHeight / 7);
            const int h = cfg::WindowHeight / 7;
            const uint8_t shade = static_cast<uint8_t>(14 + band * 6);
            r.FillRect(0, y, cfg::WindowWidth, h, Color::RGB(shade / 2, shade / 3 + 5, shade));
        }

        // Constellation sparks
        static const int STARS[][2] = {
            { 120, 90 }, { 100, 280 }, { 340, 60 }, { 420, 220 }, { 520, 140 },
            { 640, 90 }, { 780, 180 }, { 880, 60 }, { 1020, 200 }, { 1150, 120 },
            { 200, 320 }, { 360, 360 }, { 520, 300 }, { 700, 340 }, { 860, 300 },
            { 1040, 320 }, { 180, 480 }, { 320, 520 }, { 500, 460 }, { 660, 500 },
            { 820, 460 }, { 980, 520 }, { 1140, 470 }
        };

        for (const auto& star : STARS)
            r.FillRect(star[0], star[1], 3, 3, BurntGold());

        r.FillRect(80, 120, 360, 2, Ember());
        r.FillRect(460, 200, 420, 2, Ember());
        r.FillRect(cfg::WindowWidth - 500, 160, 340, 2, Ember());
    }
}

Ui::Ui(Font& font) : m_font(font)
{
    m_settingsDetail = "Refine how your realm looks, sounds, and controls.";
    m_mapPreviewSeed = PreviewSeed(m_wgChoice);
}

void Ui::SetStatusMessage(const std::string& text)
{
    m_statusMessage = text;
}

void Ui::MainMenuTick(bool upPressed, bool downPressed, bool selectPressed)
{
    static const int MENU_COUNT = 3;

    if (upPressed)
        m_mainMenuSelection = (m_mainMenuSelection + MENU_COUNT - 1) % MENU_COUNT;
    if (downPressed)
        m_mainMenuSelection = (m_mainMenuSelection + 1) % MENU_COUNT;

    if (selectPressed)
        m_mainMenuActivated = true;
}

void Ui::MainMenuRender(Renderer& r)
{
    DrawCelestialBackdrop(r);

    const int centerX = cfg::WindowWidth / 2;

    // Title panel
    const int panelW = 880;
    const int panelH = 500;
    const int panelX = (cfg::WindowWidth - panelW) / 2;
    const int panelY = 100;

    r.FillRect(panelX, panelY, panelW, panelH, Color::RGB(16, 12, 20));
    r.DrawRect(panelX, panelY, panelW, panelH, BurntGold());
    r.DrawRect(panelX + 6, panelY + 6, panelW - 12, panelH - 12, Ember());

    const std::string title = "DUNGEON DELVERS";
    int titleX = centerX - (int)title.size() * m_font.GlyphW() / 2;
    int titleY = panelY + 40;
    m_font.DrawText(r, titleX, titleY, title);

    // Buttons
    const int buttonY = panelY + 150;
    const int lineH = m_font.GlyphH() * 2 + 6;

    auto drawItem = [&](int idx, int y, const std::string& label)
    {
        const bool selected = (m_mainMenuSelection == idx);
        const int pad = 24;
        const int barX = panelX + pad;
        const int barW = panelW - pad * 2;
        const int barH = m_font.GlyphH() + 12;
        const int barY = y - 6;

        r.FillRect(barX, barY, barW, barH, selected ? Color::RGB(26, 20, 26) : Color::RGB(18, 14, 22));
        r.DrawRect(barX, barY, barW, barH, selected ? BurntGold() : Ember());

        std::string text = selected ? ("> " + label) : ("  " + label);
        int x = centerX - (int)text.size() * m_font.GlyphW() / 2;
        m_font.DrawText(r, x, y, text);
    };

    drawItem(0, buttonY, "CREATE NEW WORLD");
    drawItem(1, buttonY + lineH, "SETTINGS");
    drawItem(2, buttonY + lineH * 2, "QUIT");

    if (!m_statusMessage.empty())
    {
        int statusX = centerX - (int)m_statusMessage.size() * m_font.GlyphW() / 2;
        m_font.DrawText(r, statusX, cfg::WindowHeight - 64, m_statusMessage);
    }
}

void Ui::ClearMainMenuActivated()
{
    m_mainMenuActivated = false;
}

void Ui::SettingsTick(bool up, bool down, bool select, bool back)
{
    static const int SETTINGS_COUNT = 4;
    static const char* SETTINGS_DESCRIPTIONS[SETTINGS_COUNT] = {
        "Tune resolution, scaling, and visual effects.",
        "Balance volume, ambience, and alerts.",
        "Adjust difficulty, automation, and pacing.",
        "Remap actions to your preferred keys."
    };

    if (up)
        m_settingsSelection = (m_settingsSelection + SETTINGS_COUNT - 1) % SETTINGS_COUNT;
    if (down)
        m_settingsSelection = (m_settingsSelection + 1) % SETTINGS_COUNT;

    // Update descriptive text whenever the selection changes or is confirmed
    m_settingsDetail = SETTINGS_DESCRIPTIONS[m_settingsSelection];

    if (select)
        m_settingsDetail = SETTINGS_DESCRIPTIONS[m_settingsSelection];

    if (back)
        m_settingsBackRequested = true;
}

void Ui::SettingsRender(Renderer& r)
{
    DrawCelestialBackdrop(r);

    const int panelW = 920;
    const int panelH = 540;
    const int panelX = (cfg::WindowWidth - panelW) / 2;
    const int panelY = 70;

    r.FillRect(panelX, panelY, panelW, panelH, Color::RGB(14, 12, 22));
    r.DrawRect(panelX, panelY, panelW, panelH, BurntGold());
    r.DrawRect(panelX + 6, panelY + 6, panelW - 12, panelH - 12, Ember());

    const std::string title = "SETTINGS";
    int titleX = (cfg::WindowWidth / 2) - (int)title.size() * m_font.GlyphW() / 2;
    m_font.DrawText(r, titleX, panelY + 28, title);

    static const char* SETTINGS_LABELS[4] = {
        "VIDEO",
        "AUDIO",
        "GAME",
        "KEYBINDINGS"
    };

    int y = panelY + 100;
    const int line = m_font.GlyphH() * 2 + 6;
    for (int i = 0; i < 4; ++i)
    {
        const bool selected = (i == m_settingsSelection);

        const int rowX = panelX + 40;
        const int rowW = panelW - 80;
        const int rowH = m_font.GlyphH() + 12;
        const int rowY = y - 6;

        r.FillRect(rowX, rowY, rowW, rowH, selected ? Color::RGB(22, 18, 26) : Color::RGB(16, 12, 20));
        r.DrawRect(rowX, rowY, rowW, rowH, selected ? BurntGold() : Ember());

        std::string left = selected ? "> " : "  ";
        left += SETTINGS_LABELS[i];

        m_font.DrawText(r, rowX + 18, y, left);
        y += line;
    }

    if (m_settingsDetail.empty())
        m_settingsDetail = "Refine how your realm looks, sounds, and controls.";

    const int detailY = panelY + panelH - 90;
    int detailX = (cfg::WindowWidth / 2) - (int)m_settingsDetail.size() * m_font.GlyphW() / 2;
    m_font.DrawText(r, detailX, detailY, m_settingsDetail);

    const std::string footer = "Press ESC to return to the main menu.";
    int footerX = (cfg::WindowWidth / 2) - (int)footer.size() * m_font.GlyphW() / 2;
    m_font.DrawText(r, footerX, detailY + m_font.GlyphH() + 10, footer);
}

void Ui::ClearSettingsBackRequest()
{
    m_settingsBackRequested = false;
}

// ====================== WORLD GENERATION MENU ======================

static const char* WG_LABELS[7] = {
    "WORLD SIZE",
    "HISTORY LENGTH",
    "CIVILIZATION SATURATION",
    "SITE DENSITY",
    "WORLD VOLATILITY",
    "RESOURCE ABUNDANCE",
    "MONSTROUS POPULATION"
};

static const char* WG_VALUES[7][5] = {
    { "TINY", "SMALL", "MIDDLING", "LARGE", "VAST" },
    { "PRIMAL", "SHORT", "MIDDLING", "LONG", "ANCIENT" },
    { "SCARCE", "LOW", "MIDDLING", "DENSE", "EXCESSIVE" },
    { "SCARCE", "LOW", "MIDDLING", "DENSE", "EXCESSIVE" },
    { "STABLE", "LOW", "MIDDLING", "TURBULENT", "CHAOTIC" },
    { "SCARCE", "LOW", "MIDDLING", "DENSE", "EXCESSIVE" },
    { "SCARCE", "LOW", "MIDDLING", "DENSE", "EXCESSIVE" }
};

void Ui::WorldGenTick(bool up, bool down, bool left, bool right, bool select, bool back)
{
    const int previousWorldSize = m_wgChoice[0];
    bool settingsChanged = false;

    if (up)   m_wgRow = (m_wgRow + 6) % 7;
    if (down) m_wgRow = (m_wgRow + 1) % 7;

    if (left)
    {
        m_wgChoice[m_wgRow] = (m_wgChoice[m_wgRow] + 4) % 5;
        settingsChanged = true;
    }
    if (right)
    {
        m_wgChoice[m_wgRow] = (m_wgChoice[m_wgRow] + 1) % 5;
        settingsChanged = true;
    }

    if (m_wgChoice[0] != previousWorldSize)
    {
        m_regionBiomesReady = false;
        m_mapViewStage = MapViewStage::Region;
        m_selectedRegionX = 0;
        m_selectedRegionY = 0;
    }

    if (settingsChanged)
    {
        m_mapPreviewSeed = PreviewSeed(m_wgChoice);
        m_regionBiomesReady = false;
    }

    if (select) m_worldGenStartRequested = true;
    if (back)   m_worldGenBackRequested = true;
}

void Ui::WorldGenRender(Renderer& r)
{
    DrawCelestialBackdrop(r);

    const int panelW = 980;
    const int panelH = 520;
    const int panelX = (cfg::WindowWidth - panelW) / 2;
    const int panelY = 80;

    r.FillRect(panelX, panelY, panelW, panelH, Color::RGB(14, 12, 22));
    r.DrawRect(panelX, panelY, panelW, panelH, BurntGold());
    r.DrawRect(panelX + 6, panelY + 6, panelW - 12, panelH - 12, Ember());

    const std::string title = "WORLD GENERATION";
    int titleX = (cfg::WindowWidth / 2) - (int)title.size() * m_font.GlyphW() / 2;
    m_font.DrawText(r, titleX, panelY + 28, title);

    int y = panelY + 90;
    const int line = m_font.GlyphH() * 2 + 4;

    for (int i = 0; i < 7; ++i)
    {
        const bool selected = (i == m_wgRow);

        const int rowX = panelX + 40;
        const int rowW = panelW - 80;
        const int rowH = m_font.GlyphH() + 10;
        const int rowY = y - 4;
        r.FillRect(rowX, rowY, rowW, rowH, selected ? Color::RGB(20, 16, 26) : Color::RGB(16, 12, 20));
        r.DrawRect(rowX, rowY, rowW, rowH, selected ? BurntGold() : Ember());

        std::string left = selected ? "> " : "  ";
        left += WG_LABELS[i];

        m_font.DrawText(r, rowX + 12, y, left);
        m_font.DrawText(r, panelX + panelW - 220, y, WG_VALUES[i][m_wgChoice[i]]);

        y += line;
    }
}

void Ui::ClearWorldGenRequests()
{
    m_worldGenStartRequested = false;
    m_worldGenBackRequested = false;
}

WorldGenSettings Ui::GetWorldGenSettings() const
{
    WorldGenSettings s;
    s.worldSize = m_wgChoice[0];
    s.historyLength = m_wgChoice[1];
    s.civilizationSaturation = m_wgChoice[2];
    s.siteDensity = m_wgChoice[3];
    s.worldVolatility = m_wgChoice[4];
    s.resourceAbundance = m_wgChoice[5];
    s.monstrousPopulation = m_wgChoice[6];
    return s;
}

int Ui::RegionResolution() const
{
    return WorldResolution(m_wgChoice[0]);
}

int Ui::SampleBiome(uint8_t v) const
{
    if (v < 64) return 0;      // ocean
    if (v < 128) return 1;     // plains
    if (v < 192) return 2;     // forest
    return 3;                  // mountains
}

Color Ui::BiomeColor(int biome) const
{
    switch (biome)
    {
    case 0: return Color::RGB(12, 42, 96);
    case 1: return Color::RGB(78, 122, 64);
    case 2: return Color::RGB(36, 92, 44);
    case 3: return Color::RGB(120, 120, 120);
    default: return Color::RGB(0, 0, 0);
    }
}

void Ui::BuildRegionBiomeSummaries()
{
    const int res = RegionResolution();
    if (m_regionBiomesReady && res == m_regionResolution && m_lastMapPreviewWorldSize == m_wgChoice[0])
        return;

    m_regionResolution = res;
    m_regionBiomeSummary.assign(res * res, 0);
    m_localBlockBiomes.assign(res * res * LOCAL_BLOCKS_PER_REGION * LOCAL_BLOCKS_PER_REGION, 0);

    if (m_mapPreviewSeed == 0u)
        m_mapPreviewSeed = PreviewSeed(m_wgChoice);

    const int blockGridSize = res * LOCAL_BLOCKS_PER_REGION;
    world::NoiseParams params;
    params.scale = static_cast<float>(blockGridSize) * 1.5f;
    params.octaves = 5;
    params.persistence = 0.5f;
    params.lacunarity = 2.0f;
    params.seed = m_mapPreviewSeed;
    params.offsetX = 0.0f;
    params.offsetY = 0.0f;

    auto blockNoise = world::NormalizeToU8(world::PerlinFbm2D(blockGridSize, blockGridSize, params));

    for (int y = 0; y < blockGridSize; ++y)
    {
        for (int x = 0; x < blockGridSize; ++x)
        {
            const int biome = SampleBiome(blockNoise[y * blockGridSize + x]);
            m_localBlockBiomes[y * blockGridSize + x] = biome;
        }
    }

    for (int regionY = 0; regionY < res; ++regionY)
    {
        for (int regionX = 0; regionX < res; ++regionX)
        {
            int counts[4] = { 0, 0, 0, 0 };
            for (int by = 0; by < LOCAL_BLOCKS_PER_REGION; ++by)
            {
                for (int bx = 0; bx < LOCAL_BLOCKS_PER_REGION; ++bx)
                {
                    const int gx = regionX * LOCAL_BLOCKS_PER_REGION + bx;
                    const int gy = regionY * LOCAL_BLOCKS_PER_REGION + by;
                    const int biome = m_localBlockBiomes[gy * blockGridSize + gx];
                    if (biome >= 0 && biome < 4)
                        ++counts[biome];
                }
            }

            int bestBiome = 0;
            int bestCount = counts[0];
            for (int i = 1; i < 4; ++i)
            {
                if (counts[i] > bestCount)
                {
                    bestCount = counts[i];
                    bestBiome = i;
                }
            }

            m_regionBiomeSummary[regionY * res + regionX] = bestBiome;
        }
    }

    m_regionBiomesReady = true;
    m_lastMapPreviewWorldSize = m_wgChoice[0];
    m_mapViewStage = MapViewStage::Region;
    m_selectedRegionX = std::clamp(m_selectedRegionX, 0, res - 1);
    m_selectedRegionY = std::clamp(m_selectedRegionY, 0, res - 1);
    m_spawnConfirmed = false;
}

void Ui::GenerateLocalTerrain(int regionX, int regionY)
{
    r.FillRect(0, 0, cfg::WindowWidth, cfg::WindowHeight, Color::RGB(0, 0, 0));

void Ui::MapGenTick(bool upPressed, bool downPressed, bool leftPressed, bool rightPressed, bool confirmPressed, bool backPressed, int /*wheelDelta*/)
{
    if (m_lastMapPreviewWorldSize != m_wgChoice[0])
    {
        m_regionBiomesReady = false;
        m_mapViewStage = MapViewStage::Region;
        m_selectedRegionX = 0;
        m_selectedRegionY = 0;
    }

    BuildRegionBiomeSummaries();

    if (m_mapViewStage == MapViewStage::Region)
    {
        if (upPressed)    m_selectedRegionY = std::max(0, m_selectedRegionY - 1);
        if (downPressed)  m_selectedRegionY = std::min(RegionResolution() - 1, m_selectedRegionY + 1);
        if (leftPressed)  m_selectedRegionX = std::max(0, m_selectedRegionX - 1);
        if (rightPressed) m_selectedRegionX = std::min(RegionResolution() - 1, m_selectedRegionX + 1);

    if (m_mapPreviewReady)
    {
        SDL_Rect src{ 0, 0, m_mapPreview.Width(), m_mapPreview.Height() };
        SDL_Rect dst{ previewX, previewY, previewSize, previewSize };
        r.Blit(m_mapPreview, src, dst);
    }
    else if (m_mapViewStage == MapViewStage::Local)
    {
        const int maxBlock = LOCAL_BLOCKS_PER_REGION - 1;
        if (upPressed)    m_localSelectionBlockY = std::max(0, m_localSelectionBlockY - 1);
        if (downPressed)  m_localSelectionBlockY = std::min(maxBlock - 3, m_localSelectionBlockY + 1);
        if (leftPressed)  m_localSelectionBlockX = std::max(0, m_localSelectionBlockX - 1);
        if (rightPressed) m_localSelectionBlockX = std::min(maxBlock - 3, m_localSelectionBlockX + 1);

        if (confirmPressed)
            m_spawnConfirmed = true;

        if (backPressed)
        {
            m_mapViewStage = MapViewStage::Region;
            m_spawnConfirmed = false;
        }
    }
}

void Ui::MapGenRender(Renderer& r)
{
    const int w = cfg::WindowWidth;
    const int h = cfg::WindowHeight;
    const int resolution = WorldResolution(m_wgChoice[0]);

    BuildRegionBiomeSummaries();

    world::NoiseParams p;
    p.scale = static_cast<float>(resolution) * 2.0f;
    p.octaves = 5;
    p.persistence = 0.5f;
    p.lacunarity = 2.0f;
    p.seed = m_mapPreviewSeed;
    p.offsetX = m_mapPreviewOffsetX;
    p.offsetY = m_mapPreviewOffsetY;

    auto noise = world::PerlinFbm2D(w, h, p);
    auto gray = world::NormalizeToU8(noise);
    auto rgba = world::GrayToRGBA(gray); // size = w*h*4

    // NOTE: this requires Texture + Renderer support below (Step 4).
    if (!m_mapPreviewReady || m_mapPreview.Width() != w || m_mapPreview.Height() != h)
    {
        const int res = RegionResolution();
        const int availableW = cfg::WindowWidth - 120;
        const int availableH = cfg::WindowHeight - 140;
        const int cell = std::max(1, std::min(availableW / res, availableH / res));
        const int startX = (cfg::WindowWidth - cell * res) / 2;
        const int startY = (cfg::WindowHeight - cell * res) / 2;

        for (int y = 0; y < res; ++y)
        {
            for (int x = 0; x < res; ++x)
            {
                const int biome = m_regionBiomeSummary[y * res + x];
                r.FillRect(startX + x * cell, startY + y * cell, cell, cell, BiomeColor(biome));
            }
        }

        r.DrawRect(startX + m_selectedRegionX * cell, startY + m_selectedRegionY * cell, cell, cell, BurntGold());

        const std::string hint = "Select a region to inspect local terrain (Enter) or press ESC to return.";
        const int textX = (cfg::WindowWidth / 2) - static_cast<int>(hint.size()) * m_font.GlyphW() / 2;
        m_font.DrawText(r, textX, startY - m_font.GlyphH() * 2, hint);
    }
    else if (m_mapViewStage == MapViewStage::Local)
    {
        const int tilesPerSide = LOCAL_BLOCKS_PER_REGION * TERRAIN_TILES_PER_BLOCK;
        const int availableW = cfg::WindowWidth - 80;
        const int availableH = cfg::WindowHeight - 120;
        const int cell = std::max(1, std::min(availableW / tilesPerSide, availableH / tilesPerSide));
        const int startX = (cfg::WindowWidth - cell * tilesPerSide) / 2;
        const int startY = (cfg::WindowHeight - cell * tilesPerSide) / 2;

        for (int y = 0; y < tilesPerSide; ++y)
        {
            for (int x = 0; x < tilesPerSide; ++x)
            {
                const int biome = m_localTerrainBiomes[y * tilesPerSide + x];
                r.FillRect(startX + x * cell, startY + y * cell, cell, cell, BiomeColor(biome));
            }
        }

        const int selectionTileX = m_localSelectionBlockX * TERRAIN_TILES_PER_BLOCK;
        const int selectionTileY = m_localSelectionBlockY * TERRAIN_TILES_PER_BLOCK;
        const int selectionTiles = TERRAIN_TILES_PER_BLOCK * 4;
        r.DrawRect(startX + selectionTileX * cell, startY + selectionTileY * cell, selectionTiles * cell, selectionTiles * cell, BurntGold());

        const std::string hint = "Move to choose a 4x4 local block spawn area, Enter to confirm, ESC to pick another region.";
        const int textX = (cfg::WindowWidth / 2) - static_cast<int>(hint.size()) * m_font.GlyphW() / 2;
        m_font.DrawText(r, textX, startY - m_font.GlyphH() * 2, hint);
    }
}

void Ui::BeginMapLoading(const WorldGenSettings& settings)
{
    m_loadingSettings = settings;
    m_loadingMessage = "Forging world...";
}

void Ui::MapLoadingRender(Renderer& r)
{
    r.FillRect(0, 0, cfg::WindowWidth, cfg::WindowHeight, Color::RGB(0, 0, 0));

    const std::string title = "FORGING REALM";
    const int titleX = (cfg::WindowWidth / 2) - static_cast<int>(title.size()) * m_font.GlyphW() / 2;
    const int titleY = cfg::WindowHeight / 2 - m_font.GlyphH() * 2;
    m_font.DrawText(r, titleX, titleY, title);

    const int clampedWorldSize = std::clamp(m_loadingSettings.worldSize, 0, 4);
    std::string detail = "World size: ";
    detail += WG_VALUES[0][clampedWorldSize];

    const int detailX = (cfg::WindowWidth / 2) - static_cast<int>(detail.size()) * m_font.GlyphW() / 2;
    m_font.DrawText(r, detailX, titleY + m_font.GlyphH() * 2, detail);

    if (!m_loadingMessage.empty())
    {
        const int msgX = (cfg::WindowWidth / 2) - static_cast<int>(m_loadingMessage.size()) * m_font.GlyphW() / 2;
        m_font.DrawText(r, msgX, titleY + m_font.GlyphH() * 4, m_loadingMessage);
    }
}

