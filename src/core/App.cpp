// src/core/App.cpp
#include "core/App.h"
#include "core/Config.h"
#include "core/Log.h"
#include "core/GameState.h"
#include "input/Input.h"
#include "gfx/Renderer.h"
#include "gfx/Font.h"
#include "ui/Ui.h"

#include <SDL.h>
#include <string>

static uint64_t NowCounter() { return static_cast<uint64_t>(SDL_GetPerformanceCounter()); }
static double CounterToSeconds(uint64_t delta)
{
    const double freq = static_cast<double>(SDL_GetPerformanceFrequency());
    return static_cast<double>(delta) / freq;
}

App::App() = default;

App::~App()
{
    Shutdown();
}

bool App::Init()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
    {
        logx::Error(std::string("SDL_Init failed: ") + SDL_GetError());
        return false;
    }

    m_window = SDL_CreateWindow(
        "DungeonCore",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        cfg::WindowWidth, cfg::WindowHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP
    );

    if (!m_window)
    {
        logx::Error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
        return false;
    }

    // Use hardware acceleration + vsync
    m_sdlRenderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_sdlRenderer)
    {
        logx::Error(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());
        return false;
    }

    SDL_RenderSetLogicalSize(m_sdlRenderer, cfg::WindowWidth, cfg::WindowHeight);

    m_input = new Input();
    m_renderer = new Renderer(m_sdlRenderer);
    m_font = new Font(*m_renderer);
    if (!m_font->LoadAtlasBMP(*m_renderer, cfg::FontAtlasPath, cfg::FontGlyphPx, cfg::FontGlyphPx))
    {
        logx::Warn("Font atlas load failed; text will not render.");
    }

    m_ui = new Ui(*m_font);

    m_statusMessage = "Forge a new realm beneath a celestial sky.";
    m_ui->SetStatusMessage(m_statusMessage);

    m_running = true;
    logx::Info("Init OK");
    return true;
}

void App::Shutdown()
{
    delete m_ui; m_ui = nullptr;
    delete m_font; m_font = nullptr;
    delete m_renderer; m_renderer = nullptr;
    delete m_input; m_input = nullptr;

    if (m_sdlRenderer) { SDL_DestroyRenderer(m_sdlRenderer); m_sdlRenderer = nullptr; }
    if (m_window) { SDL_DestroyWindow(m_window); m_window = nullptr; }

    SDL_Quit();
}

int App::Run()
{
    if (!Init())
        return 1;

    uint64_t last = NowCounter();

    while (m_running)
    {
        const uint64_t now = NowCounter();
        const uint64_t delta = now - last;
        last = now;

        const double dt = CounterToSeconds(delta);
        (void)dt;

        m_input->BeginFrame();
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                m_running = false;
            else
                m_input->ProcessEvent(e);
        }

        if (m_input->PressedOnce(SDLK_q))
            m_running = false;

        if (m_state == GameState::MainMenu && m_input->PressedOnce(SDLK_ESCAPE))
            m_running = false;

        // State-specific input
        if (m_state == GameState::MainMenu)
        {
            const bool up =
                m_input->PressedOnce(SDLK_UP) || m_input->PressedOnce(SDLK_w);
            const bool down =
                m_input->PressedOnce(SDLK_DOWN) || m_input->PressedOnce(SDLK_s);
            const bool select =
                m_input->PressedOnce(SDLK_RETURN) || m_input->PressedOnce(SDLK_KP_ENTER);

            m_ui->MainMenuTick(up, down, select);

            if (m_ui->MainMenuActivated())
            {
                const int sel = m_ui->MainMenuSelection();
                m_ui->ClearMainMenuActivated();

                if (sel == 0)
                {
                    m_state = GameState::WorldGen;
                }
                else if (sel == 1)
                {
                    m_state = GameState::Settings;
                }
                else if (sel == 2)
                {
                    m_running = false;
                }
            }
        }

        else if (m_state == GameState::Settings)
        {
            const bool up = m_input->PressedOnce(SDLK_UP) || m_input->PressedOnce(SDLK_w);
            const bool down = m_input->PressedOnce(SDLK_DOWN) || m_input->PressedOnce(SDLK_s);
            const bool select = m_input->PressedOnce(SDLK_RETURN) || m_input->PressedOnce(SDLK_KP_ENTER);
            const bool back = m_input->PressedOnce(SDLK_ESCAPE);

            m_ui->SettingsTick(up, down, select, back);

            if (m_ui->SettingsBackRequested())
            {
                m_ui->ClearSettingsBackRequest();
                m_state = GameState::MainMenu;
            }
        }

        else if (m_state == GameState::WorldGen)
        {
            const bool up = m_input->PressedOnce(SDLK_UP) || m_input->PressedOnce(SDLK_w);
            const bool down = m_input->PressedOnce(SDLK_DOWN) || m_input->PressedOnce(SDLK_s);
            const bool left = m_input->PressedOnce(SDLK_LEFT) || m_input->PressedOnce(SDLK_a);
            const bool right = m_input->PressedOnce(SDLK_RIGHT) || m_input->PressedOnce(SDLK_d);

            const bool select = m_input->PressedOnce(SDLK_RETURN) || m_input->PressedOnce(SDLK_KP_ENTER);
            const bool back = m_input->PressedOnce(SDLK_ESCAPE);

            m_ui->WorldGenTick(up, down, left, right, select, back);

            if (m_ui->WorldGenBackRequested())
            {
                m_ui->ClearWorldGenRequests();
                m_ui->SetStatusMessage(m_statusMessage);
                m_state = GameState::MainMenu;
            }
            else if (m_ui->WorldGenStartRequested())
            {
                m_pendingSettings = m_ui->GetWorldGenSettings();
                m_ui->ClearWorldGenRequests();
                m_state = GameState::MapGenSelection;
            }
        }

        else if (m_state == GameState::MapGenSelection)
        {
            const bool up = m_input->Down(SDLK_w) || m_input->Down(SDLK_UP);
            const bool down = m_input->Down(SDLK_s) || m_input->Down(SDLK_DOWN);
            const bool left = m_input->Down(SDLK_a) || m_input->Down(SDLK_LEFT);
            const bool right = m_input->Down(SDLK_d) || m_input->Down(SDLK_RIGHT);
            const bool confirm = m_input->PressedOnce(SDLK_RETURN) || m_input->PressedOnce(SDLK_KP_ENTER);
            const bool back = m_input->PressedOnce(SDLK_ESCAPE);

            m_ui->MapGenTick(up, down, left, right, confirm, back, m_input->WheelY());

            if (m_ui->MapSelectionComplete())
            {
                m_state = GameState::MapLoading;
                m_ui->BeginMapLoading(m_pendingSettings);
                m_ui->ClearMapSelectionFlags();
            }
            else if (m_ui->MapGenBackRequested())
            {
                m_ui->ClearMapSelectionFlags();
                m_state = GameState::WorldGen;
            }
        }

        else if (m_state == GameState::MapLoading)
        {
            // Future loading logic can live here; currently passive
        }

        Render();
    }

    return 0;
}

void App::Render()
{
    m_renderer->Clear();

    if (m_state == GameState::MainMenu)
    {
        m_ui->MainMenuRender(*m_renderer);
    }
    else if (m_state == GameState::Settings)
    {
        m_ui->SettingsRender(*m_renderer);
    }
    else if (m_state == GameState::WorldGen)
    {
        m_ui->WorldGenRender(*m_renderer);
    }
    else if (m_state == GameState::MapGenSelection)
    {
        m_ui->MapGenRender(*m_renderer);
    }
    else if (m_state == GameState::MapLoading)
    {
        m_ui->MapLoadingRender(*m_renderer);
    }

    m_renderer->Present();
}
