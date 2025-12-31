#include "ui/Ui.h"
#include "gfx/Font.h"
#include "gfx/Renderer.h"
#include "gfx/Color.h"
#include "core/Config.h"
#include "gfx/Texture.h"
#include "world/Noise.h"

#include <cstdint>
#include <SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <random>
#include <string>
#include <vector>

namespace
{
    Color BurntGold() { return Color::RGB(196, 146, 64); }
    Color Ember()     { return Color::RGB(122, 82, 44); }
    Color DeepNight() { return Color::RGB(8, 10, 18); }

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

    if (up)   m_wgRow = (m_wgRow + 6) % 7;
    if (down) m_wgRow = (m_wgRow + 1) % 7;

    if (left)
        m_wgChoice[m_wgRow] = (m_wgChoice[m_wgRow] + 4) % 5;
    if (right)
        m_wgChoice[m_wgRow] = (m_wgChoice[m_wgRow] + 1) % 5;

    if (m_wgChoice[0] != previousWorldSize)
        m_mapPreviewReady = false;

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

void Ui::MapGenTick(bool upPressed, bool downPressed, bool leftPressed, bool rightPressed, int wheelDelta)
{
    const float moveStep = 48.0f / std::max(1.0f, m_mapPreviewZoom);
    bool moved = false;
    bool zoomed = false;

    if (leftPressed)
    {
        m_mapPreviewOffsetX -= moveStep;
        moved = true;
    }
    if (rightPressed)
    {
        m_mapPreviewOffsetX += moveStep;
        moved = true;
    }
    if (upPressed)
    {
        m_mapPreviewOffsetY -= moveStep;
        moved = true;
    }
    if (downPressed)
    {
        m_mapPreviewOffsetY += moveStep;
        moved = true;
    }

    if (wheelDelta != 0)
    {
        const float zoomStep = 1.12f;
        float factor = (wheelDelta > 0) ? zoomStep : (1.0f / zoomStep);

        for (int i = 1; i < std::abs(wheelDelta); ++i)
            factor *= (wheelDelta > 0) ? zoomStep : (1.0f / zoomStep);

        m_mapPreviewZoom = std::clamp(m_mapPreviewZoom * factor, 0.25f, 6.0f);
        zoomed = true;
    }

    if (moved || zoomed)
        m_mapPreviewReady = false;
}

void Ui::MapGenRender(Renderer& r)
{
    DrawCelestialBackdrop(r);

    if (m_lastMapPreviewWorldSize != m_wgChoice[0])
    {
        m_mapPreviewReady = false;
        m_mapPreviewOffsetX = 0.0f;
        m_mapPreviewOffsetY = 0.0f;
        m_mapPreviewZoom = 1.0f;
    }

    if (!m_mapPreviewReady)
        GenerateMapPreview(r);

    const int previewSize = std::min(cfg::WindowHeight - 80, 680);
    const int previewX = (cfg::WindowWidth - previewSize) / 2;
    const int previewY = (cfg::WindowHeight - previewSize) / 2;

    // Soft shadow and frame to give the preview a focal feel
    r.FillRect(previewX + 14, previewY + 18, previewSize, previewSize, Color::RGB(6, 6, 12));
    r.FillRect(previewX, previewY, previewSize, previewSize, Color::RGB(16, 12, 20));
    r.DrawRect(previewX, previewY, previewSize, previewSize, Ember());
    r.DrawRect(previewX + 8, previewY + 8, previewSize - 16, previewSize - 16, BurntGold());

    if (m_mapPreviewReady)
    {
        SDL_Rect src{ 0, 0, m_mapPreview.Width(), m_mapPreview.Height() };
        SDL_Rect dst{ previewX + 16, previewY + 16, previewSize - 32, previewSize - 32 };
        r.Blit(m_mapPreview, src, dst);
    }

    // Overlay card for controls and context; intentionally overlaps the centered preview
    const int cardW = 360;
    const int cardH = 220;
    const int cardX = cfg::WindowWidth - cardW - 28;
    const int cardY = 32;

    r.FillRect(cardX, cardY, cardW, cardH, Color::RGB(14, 12, 22));
    r.DrawRect(cardX, cardY, cardW, cardH, BurntGold());
    r.DrawRect(cardX + 4, cardY + 4, cardW - 8, cardH - 8, Ember());

    m_font.DrawText(r, cardX + 14, cardY + 14, "MAP GENERATION");
    m_font.DrawText(r, cardX + 14, cardY + 46, "Pan: WASD or arrows");
    m_font.DrawText(r, cardX + 14, cardY + 46 + m_font.GlyphH() + 6, "Zoom: mouse wheel");
    m_font.DrawText(r, cardX + 14, cardY + cardH - 28, "ESC to return");
}

void Ui::GenerateMapPreview(Renderer& r)
{
    static const int WORLD_SIZE_TO_RESOLUTION[5] = { 256, 384, 512, 640, 768 };
    const int w = WORLD_SIZE_TO_RESOLUTION[m_wgChoice[0]];
    const int h = WORLD_SIZE_TO_RESOLUTION[m_wgChoice[0]];

    std::random_device rd;
    uint32_t seed = (uint32_t)rd() ^ ((uint32_t)rd() << 16);

    world::NoiseParams p;
    p.scale = 128.0f;
    p.octaves = 5;
    p.persistence = 0.5f;
    p.lacunarity = 2.0f;
    p.seed = seed;
    p.offsetX = m_mapPreviewOffsetX;
    p.offsetY = m_mapPreviewOffsetY;

    auto noise = world::PerlinFbm2D(w, h, p);
    auto gray = world::NormalizeToU8(noise);
    auto rgba = world::GrayToRGBA(gray); // size = w*h*4

    // NOTE: this requires Texture + Renderer support below (Step 4).
    if (!m_mapPreviewReady || m_mapPreview.Width() != w || m_mapPreview.Height() != h)
    {
        if (!m_mapPreview.CreateRGBAStreaming(r.Raw(), w, h))
        {
            SetStatusMessage("Failed to create map preview texture");
            return;
        }
        m_mapPreviewReady = true;
    }

    if (!m_mapPreview.UpdateRGBA(rgba.data(), w * 4))
    {
        SetStatusMessage("Failed to upload map preview pixels");
        return;
    }

    m_lastMapPreviewWorldSize = m_wgChoice[0];
    SetStatusMessage("Map preview generated");
}

