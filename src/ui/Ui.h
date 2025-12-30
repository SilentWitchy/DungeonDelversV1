#pragma once
#include <string>
#include "world/WorldGenSettings.h"

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

    void WorldGenTick(bool upPressed, bool downPressed, bool leftPressed, bool rightPressed,
        bool selectPressed, bool backPressed);
    void WorldGenRender(Renderer& r);

    bool WorldGenStartRequested() const { return m_worldGenStartRequested; }
    bool WorldGenBackRequested() const { return m_worldGenBackRequested; }
    void ClearWorldGenRequests();

    WorldGenSettings GetWorldGenSettings() const;

    void SetStatusMessage(const std::string& text);

private:
    Font& m_font;

    // Main menu state
    int  m_mainMenuSelection = 0; // 0 = New World, 1 = Quit
    bool m_mainMenuActivated = false;

    int m_wgRow = 0;                 // which of the 7 options (0..6)
    int m_wgChoice[7] = { 2,2,2,2,2,2,2 }; // default to "Middle" option
    bool m_worldGenStartRequested = false;
    bool m_worldGenBackRequested = false;

    std::string m_statusMessage;
};
