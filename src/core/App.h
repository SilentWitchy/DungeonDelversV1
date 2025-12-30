// src/core/App.h
#pragma once
#include <cstdint>
#include "core/GameState.h"
#include <string>
#include "world/WorldGenSettings.h"

struct SDL_Window;
struct SDL_Renderer;

class Input;
class Renderer;
class Font;
class Ui;

class App
{
public:
    App();
    ~App();

    int Run();

private:
    bool Init();
    void Shutdown();

    void Render();

private:
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_sdlRenderer = nullptr;

    Input* m_input = nullptr;
    Renderer* m_renderer = nullptr;
    Font* m_font = nullptr;
    Ui* m_ui = nullptr;

    GameState m_state = GameState::MainMenu;

    WorldGenSettings m_pendingSettings{};
    std::string m_statusMessage;

    bool m_running = false;
};

