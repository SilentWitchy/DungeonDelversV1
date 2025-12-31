#pragma once
#include <string>
#include "world/WorldGenSettings.h"
#include "gfx/Texture.h"

class Font;
class Renderer;

class Ui
{
public:
    explicit Ui(Font& font);

    void MainMenuTick(bool upPressed, bool downPressed, bool selectPressed);
    void MainMenuRender(Renderer& r);
    int  MainMenuSelection() const { return m_mainMenuSelection; }
    bool MainMenuActivated() const { return m_mainMenuActivated; }
    void ClearMainMenuActivated();

    void SettingsTick(bool upPressed, bool downPressed, bool selectPressed, bool backPressed);
    void SettingsRender(Renderer& r);
    bool SettingsBackRequested() const { return m_settingsBackRequested; }
    void ClearSettingsBackRequest();

    void WorldGenTick(bool upPressed, bool downPressed, bool leftPressed, bool rightPressed,
        bool selectPressed, bool backPressed);
    void WorldGenRender(Renderer& r);

    void MapGenTick(bool upPressed, bool downPressed, bool leftPressed, bool rightPressed, int wheelDelta);
    void MapGenRender(Renderer& r);

    bool WorldGenStartRequested() const { return m_worldGenStartRequested; }
    bool WorldGenBackRequested() const { return m_worldGenBackRequested; }
    void ClearWorldGenRequests();

    WorldGenSettings GetWorldGenSettings() const;

    void SetStatusMessage(const std::string& text);

private:
    Font& m_font;

    // Main menu state
    int  m_mainMenuSelection = 0; // 0 = New World, 1 = Settings, 2 = Quit
    bool m_mainMenuActivated = false;

    // Settings menu state
    int  m_settingsSelection = 0;
    bool m_settingsBackRequested = false;
    std::string m_settingsDetail;

    int m_wgRow = 0;                 // which of the 7 options (0..6)
    int m_wgChoice[7] = { 2,2,2,2,2,2,2 }; // default to "Middle" option
    bool m_worldGenStartRequested = false;
    bool m_worldGenBackRequested = false;

    std::string m_statusMessage;

    void GenerateMapPreview(Renderer& r);
    Texture m_mapPreview;
    bool m_mapPreviewReady = false;
    int m_lastMapPreviewWorldSize = -1;
    float m_mapPreviewOffsetX = 0.0f;
    float m_mapPreviewOffsetY = 0.0f;
    float m_mapPreviewZoom = 1.0f;
};
