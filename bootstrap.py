#!/usr/bin/env python3
import os
import struct
from pathlib import Path

PROJECT_NAME = "DungeonCore"

# A small public-domain 8x8 font table (subset) embedded here as bytes.
# We'll generate a full 16x16 atlas by scaling these 8x8 glyphs 2x.
#
# For simplicity, we define printable ASCII 32..126 with a basic fallback:
# - Letters/digits/punctuation: minimal blocky font (not beautiful, but functional)
# - Unknown glyphs -> '?'
#
# If you want a nicer font later, you can swap the atlas BMP and keep the engine unchanged.

def glyph8x8_for_char(ch: int):
    # Very small, readable-ish 5x7 patterns centered into 8x8.
    # This is intentionally minimal; it’s enough for menus/debug text.
    # Each row is an 8-bit mask (MSB on left).
    c = chr(ch)
    empty = [0]*8

    def rows_from_5x7(pattern_5x7):
        # pattern_5x7: list[str] len 7, each 5 chars '0'/'1'
        # center in 8x8: left pad 1, top pad 0, bottom pad 1
        out = []
        for r in range(7):
            bits5 = pattern_5x7[r]
            b = 0
            for i in range(5):
                if bits5[i] == '1':
                    b |= (1 << (7 - (i+1)))  # shift into columns 1..5
            out.append(b)
        out.append(0)
        return out

    # Space
    if c == ' ':
        return empty

    # Digits (simple)
    digits = {
        '0': ["01110","10001","10011","10101","11001","10001","01110"],
        '1': ["00100","01100","00100","00100","00100","00100","01110"],
        '2': ["01110","10001","00001","00010","00100","01000","11111"],
        '3': ["11110","00001","00001","01110","00001","00001","11110"],
        '4': ["00010","00110","01010","10010","11111","00010","00010"],
        '5': ["11111","10000","11110","00001","00001","10001","01110"],
        '6': ["00110","01000","10000","11110","10001","10001","01110"],
        '7': ["11111","00001","00010","00100","01000","01000","01000"],
        '8': ["01110","10001","10001","01110","10001","10001","01110"],
        '9': ["01110","10001","10001","01111","00001","00010","01100"],
    }
    if c in digits:
        return rows_from_5x7(digits[c])

    # Uppercase letters (subset is enough; lowercase will map to uppercase)
    letters = {
        'A': ["01110","10001","10001","11111","10001","10001","10001"],
        'B': ["11110","10001","10001","11110","10001","10001","11110"],
        'C': ["01110","10001","10000","10000","10000","10001","01110"],
        'D': ["11100","10010","10001","10001","10001","10010","11100"],
        'E': ["11111","10000","10000","11110","10000","10000","11111"],
        'F': ["11111","10000","10000","11110","10000","10000","10000"],
        'G': ["01110","10001","10000","10111","10001","10001","01110"],
        'H': ["10001","10001","10001","11111","10001","10001","10001"],
        'I': ["01110","00100","00100","00100","00100","00100","01110"],
        'J': ["00111","00010","00010","00010","00010","10010","01100"],
        'K': ["10001","10010","10100","11000","10100","10010","10001"],
        'L': ["10000","10000","10000","10000","10000","10000","11111"],
        'M': ["10001","11011","10101","10101","10001","10001","10001"],
        'N': ["10001","11001","10101","10011","10001","10001","10001"],
        'O': ["01110","10001","10001","10001","10001","10001","01110"],
        'P': ["11110","10001","10001","11110","10000","10000","10000"],
        'Q': ["01110","10001","10001","10001","10101","10010","01101"],
        'R': ["11110","10001","10001","11110","10100","10010","10001"],
        'S': ["01111","10000","10000","01110","00001","00001","11110"],
        'T': ["11111","00100","00100","00100","00100","00100","00100"],
        'U': ["10001","10001","10001","10001","10001","10001","01110"],
        'V': ["10001","10001","10001","10001","10001","01010","00100"],
        'W': ["10001","10001","10001","10101","10101","11011","10001"],
        'X': ["10001","10001","01010","00100","01010","10001","10001"],
        'Y': ["10001","10001","01010","00100","00100","00100","00100"],
        'Z': ["11111","00001","00010","00100","01000","10000","11111"],
    }

    if 'a' <= c <= 'z':
        c = c.upper()

    if c in letters:
        return rows_from_5x7(letters[c])

    punct = {
        '.': ["00000","00000","00000","00000","00000","00100","00100"],
        ',': ["00000","00000","00000","00000","00100","00100","01000"],
        '!': ["00100","00100","00100","00100","00100","00000","00100"],
        '?': ["01110","10001","00001","00010","00100","00000","00100"],
        ':': ["00000","00100","00100","00000","00100","00100","00000"],
        '-': ["00000","00000","00000","11111","00000","00000","00000"],
        '+': ["00000","00100","00100","11111","00100","00100","00000"],
        '/': ["00001","00010","00100","01000","10000","00000","00000"],
        '(': ["00010","00100","01000","01000","01000","00100","00010"],
        ')': ["01000","00100","00010","00010","00010","00100","01000"],
        '[': ["01110","01000","01000","01000","01000","01000","01110"],
        ']': ["01110","00010","00010","00010","00010","00010","01110"],
    }
    if c in punct:
        return rows_from_5x7(punct[c])

    # Fallback '?'
    return rows_from_5x7(punct['?'])

def scale_8x8_to_16x16(rows8):
    # rows8: list of 8 bytes (each 8 bits)
    # return rows16: list of 16 uint16 (each 16 bits)
    rows16 = []
    for r in range(8):
        b = rows8[r]
        # expand horizontally 2x: each bit becomes 2 bits
        out16 = 0
        for col in range(8):
            bit = (b >> (7 - col)) & 1
            if bit:
                # set two bits in 16-bit row
                out16 |= (1 << (15 - (col*2)))
                out16 |= (1 << (15 - (col*2 + 1)))
        rows16.append(out16)
        rows16.append(out16)  # vertical 2x
    return rows16

def write_bmp_32(path: Path, width: int, height: int, pixels_rgba):
    # pixels_rgba: list of (r,g,b,a) row-major, top-left first
    # Write uncompressed 32-bit BMP (BI_RGB). BMP is bottom-up by default.
    row_bytes = width * 4
    image_size = row_bytes * height
    file_size = 14 + 40 + image_size

    # BITMAPFILEHEADER
    bfType = b'BM'
    bfSize = file_size
    bfReserved1 = 0
    bfReserved2 = 0
    bfOffBits = 14 + 40

    # BITMAPINFOHEADER
    biSize = 40
    biWidth = width
    biHeight = height  # positive => bottom-up
    biPlanes = 1
    biBitCount = 32
    biCompression = 0  # BI_RGB
    biSizeImage = image_size
    biXPelsPerMeter = 2835
    biYPelsPerMeter = 2835
    biClrUsed = 0
    biClrImportant = 0

    with path.open('wb') as f:
        f.write(struct.pack('<2sIHHI', bfType, bfSize, bfReserved1, bfReserved2, bfOffBits))
        f.write(struct.pack('<IIIHHIIIIII',
                            biSize, biWidth, biHeight, biPlanes, biBitCount, biCompression,
                            biSizeImage, biXPelsPerMeter, biYPelsPerMeter, biClrUsed, biClrImportant))
        # Write pixel data bottom-up. BMP expects BGRA ordering for 32-bit BI_RGB in practice.
        for y in range(height-1, -1, -1):
            row_start = y * width
            for x in range(width):
                r, g, b, a = pixels_rgba[row_start + x]
                f.write(struct.pack('<BBBB', b, g, r, a))

def generate_font_atlas_bmp(out_path: Path):
    # 16x16 per glyph, 16 columns x 8 rows => 256 glyph slots
    cell = 16
    cols = 16
    rows = 8
    atlas_w = cols * cell
    atlas_h = rows * cell

    # black background, white glyph pixels
    bg = (0, 0, 0, 255)
    fg = (255, 255, 255, 255)
    pixels = [bg] * (atlas_w * atlas_h)

    # Put ASCII 0..127 in the first 128 slots. The rest blank.
    for code in range(128):
        gx = (code % cols) * cell
        gy = (code // cols) * cell
        rows8 = glyph8x8_for_char(code if 32 <= code <= 126 else ord('?') if code not in (0, 9, 10, 13) else ord(' '))
        rows16 = scale_8x8_to_16x16(rows8)
        for ry in range(16):
            rowbits = rows16[ry]
            for rx in range(16):
                bit = (rowbits >> (15 - rx)) & 1
                if bit:
                    x = gx + rx
                    y = gy + ry
                    pixels[y * atlas_w + x] = fg

    out_path.parent.mkdir(parents=True, exist_ok=True)
    write_bmp_32(out_path, atlas_w, atlas_h, pixels)

def write_text(path: Path, text: str):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding='utf-8', newline='\n')

def main():
    root = Path(".").resolve()

    # Create folder structure
    for p in [
        root / "assets" / "fonts",
        root / "src" / "core",
        root / "src" / "gfx",
        root / "src" / "input",
        root / "src" / "ui",
        root / "src" / "world",
        root / "tools",
    ]:
        p.mkdir(parents=True, exist_ok=True)

    # Generate bitmap font atlas
    generate_font_atlas_bmp(root / "assets" / "fonts" / "font16x16.bmp")

    # Write files
    write_text(root / "README.md", README_MD)
    write_text(root / "CMakeLists.txt", CMAKELISTS)
    write_text(root / "src" / "main.cpp", MAIN_CPP)

    write_text(root / "src" / "core" / "Config.h", CONFIG_H)
    write_text(root / "src" / "core" / "Log.h", LOG_H)
    write_text(root / "src" / "core" / "Log.cpp", LOG_CPP)
    write_text(root / "src" / "core" / "App.h", APP_H)
    write_text(root / "src" / "core" / "App.cpp", APP_CPP)

    write_text(root / "src" / "gfx" / "Color.h", COLOR_H)
    write_text(root / "src" / "gfx" / "Texture.h", TEXTURE_H)
    write_text(root / "src" / "gfx" / "Texture.cpp", TEXTURE_CPP)
    write_text(root / "src" / "gfx" / "Renderer.h", RENDERER_H)
    write_text(root / "src" / "gfx" / "Renderer.cpp", RENDERER_CPP)
    write_text(root / "src" / "gfx" / "Font.h", FONT_H)
    write_text(root / "src" / "gfx" / "Font.cpp", FONT_CPP)

    write_text(root / "src" / "input" / "Input.h", INPUT_H)
    write_text(root / "src" / "input" / "Input.cpp", INPUT_CPP)

    write_text(root / "src" / "ui" / "Ui.h", UI_H)
    write_text(root / "src" / "ui" / "Ui.cpp", UI_CPP)

    write_text(root / "src" / "world" / "Tiles.h", TILES_H)
    write_text(root / "src" / "world" / "World.h", WORLD_H)
    write_text(root / "src" / "world" / "World.cpp", WORLD_CPP)
    write_text(root / "src" / "world" / "DungeonCore.h", CORE_H)
    write_text(root / "src" / "world" / "DungeonCore.cpp", CORE_CPP)

    print("Bootstrap complete.")
    print("Generated assets/fonts/font16x16.bmp")
    print("Next: configure/build with CMake (SDL2 required).")

# ---------------- FILE CONTENTS ----------------

README_MD = r"""# DungeonCore (C++ / SDL2)

A minimal-dependency foundation for a Dwarf Fortress–style game where the player is a sentient Dungeon Core.

## Dependency
- SDL2 (window/input/2D rendering)

## Controls
- WASD / Arrow keys: move camera
- ESC: toggle menu
- Q: quit

## Build (CMake)
Typical:
- Configure with your IDE (CLion) or command line
- Ensure SDL2 is discoverable by CMake

If using vcpkg:
- Install: `vcpkg install sdl2`
- Configure with toolchain:
  `-DCMAKE_TOOLCHAIN_FILE=.../vcpkg/scripts/buildsystems/vcpkg.cmake`
"""

CMAKELISTS = r"""cmake_minimum_required(VERSION 3.20)
project(DungeonCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(DungeonCore
    src/main.cpp
    src/core/App.cpp
    src/core/Log.cpp
    src/gfx/Texture.cpp
    src/gfx/Renderer.cpp
    src/gfx/Font.cpp
    src/input/Input.cpp
    src/ui/Ui.cpp
    src/world/World.cpp
    src/world/DungeonCore.cpp
)

target_include_directories(DungeonCore PRIVATE src)

# SDL2
# Works with:
# - vcpkg (preferred): find_package(SDL2 CONFIG REQUIRED)
# - system installs:  find_package(SDL2 REQUIRED)
find_package(SDL2 CONFIG QUIET)

if(SDL2_FOUND)
    message(STATUS "Found SDL2 via CONFIG.")
    target_link_libraries(DungeonCore PRIVATE SDL2::SDL2 SDL2::SDL2main)
else()
    message(STATUS "SDL2 CONFIG not found; trying MODULE mode.")
    find_package(SDL2 REQUIRED)
    target_include_directories(DungeonCore PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(DungeonCore PRIVATE ${SDL2_LIBRARIES})
endif()

# Copy assets next to the binary (simple dev workflow)
add_custom_command(TARGET DungeonCore POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:DungeonCore>/assets
)
"""

MAIN_CPP = r"""#include "core/App.h"

int main(int argc, char** argv)
{
    (void)argc; (void)argv;
    App app;
    return app.Run();
}
"""

CONFIG_H = r"""#pragma once
#include <cstdint>

namespace cfg
{
    // Window
    constexpr int WindowWidth  = 1280;
    constexpr int WindowHeight = 720;

    // Rendering
    constexpr int TileSizePx = 16;           // tiles are 16x16 pixels
    constexpr int FontGlyphPx = 16;          // font cell size (16x16)
    constexpr const char* FontAtlasPath = "assets/fonts/font16x16.bmp";

    // World
    constexpr int WorldW = 80;
    constexpr int WorldH = 45;
}
"""

LOG_H = r"""#pragma once
#include <string>

namespace logx
{
    void Info(const std::string& s);
    void Warn(const std::string& s);
    void Error(const std::string& s);
}
"""

LOG_CPP = r"""#include "core/Log.h"
#include <iostream>

namespace logx
{
    void Info(const std::string& s)  { std::cout  << "[INFO] "  << s << "\n"; }
    void Warn(const std::string& s)  { std::cout  << "[WARN] "  << s << "\n"; }
    void Error(const std::string& s) { std::cerr  << "[ERROR] " << s << "\n"; }
}
"""

APP_H = r"""#pragma once
#include <cstdint>

struct SDL_Window;
struct SDL_Renderer;

class Input;
class Renderer;
class Font;
class World;
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

    void Tick(double dt);
    void Render();

private:
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_sdlRenderer = nullptr;

    Input*   m_input = nullptr;
    Renderer* m_renderer = nullptr;
    Font*    m_font = nullptr;
    World*   m_world = nullptr;
    Ui*      m_ui = nullptr;

    bool m_running = false;
    int  m_camX = 0; // camera in tiles
    int  m_camY = 0;
};
"""

APP_CPP = r"""#include "core/App.h"
#include "core/Config.h"
#include "core/Log.h"
#include "input/Input.h"
#include "gfx/Renderer.h"
#include "gfx/Font.h"
#include "world/World.h"
#include "ui/Ui.h"

#include <SDL.h>
#include <algorithm>
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
        SDL_WINDOW_SHOWN
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

    m_input = new Input();
    m_renderer = new Renderer(m_sdlRenderer);
    m_font = new Font(*m_renderer);
    if (!m_font->LoadAtlasBMP(cfg::FontAtlasPath, cfg::FontGlyphPx, cfg::FontGlyphPx))
    {
        logx::Warn("Font atlas load failed; text will still render but may appear blank depending on platform.");
    }

    m_world = new World();
    m_ui = new Ui(*m_font);

    m_running = true;
    logx::Info("Init OK");
    return true;
}

void App::Shutdown()
{
    delete m_ui; m_ui = nullptr;
    delete m_world; m_world = nullptr;
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
        // timing
        const uint64_t now = NowCounter();
        const double dt = CounterToSeconds(now - last);
        last = now;

        // input
        m_input->BeginFrame();
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT) m_running = false;
            m_input->ProcessEvent(e);
        }

        if (m_input->PressedOnce(SDLK_q))
            m_running = false;

        if (m_input->PressedOnce(SDLK_ESCAPE))
            m_ui->ToggleMenu();

        // camera movement in tile units
        const int move = 1;
        if (!m_ui->IsMenuOpen())
        {
            if (m_input->Down(SDLK_w) || m_input->Down(SDLK_UP))    m_camY -= move;
            if (m_input->Down(SDLK_s) || m_input->Down(SDLK_DOWN))  m_camY += move;
            if (m_input->Down(SDLK_a) || m_input->Down(SDLK_LEFT))  m_camX -= move;
            if (m_input->Down(SDLK_d) || m_input->Down(SDLK_RIGHT)) m_camX += move;
        }

        // clamp camera to world bounds
        m_camX = std::clamp(m_camX, 0, cfg::WorldW - 1);
        m_camY = std::clamp(m_camY, 0, cfg::WorldH - 1);

        Tick(dt);
        Render();
    }

    return 0;
}

void App::Tick(double dt)
{
    (void)dt;
    m_world->Tick();
    m_ui->Tick();
}

void App::Render()
{
    m_renderer->Clear();

    m_world->Render(*m_renderer, *m_font, m_camX, m_camY);

    // HUD line
    m_font->DrawText(*m_renderer, 8, 8,
        "DungeonCore - WASD/Arrows move camera | ESC menu | Q quit");

    m_ui->Render(*m_renderer);

    m_renderer->Present();
}
"""

COLOR_H = r"""#pragma once
#include <cstdint>

struct Color
{
    uint8_t r=0, g=0, b=0, a=255;

    static Color RGB(uint8_t rr, uint8_t gg, uint8_t bb) { return {rr, gg, bb, 255}; }
    static Color RGBA(uint8_t rr, uint8_t gg, uint8_t bb, uint8_t aa) { return {rr, gg, bb, aa}; }
};
"""

TEXTURE_H = r"""#pragma once
#include <string>

struct SDL_Texture;
struct SDL_Renderer;
struct SDL_Surface;

class Texture
{
public:
    Texture() = default;
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    bool LoadBMP(SDL_Renderer* r, const std::string& path, bool colorKeyBlack);
    void Destroy();

    SDL_Texture* Get() const { return m_tex; }
    int Width() const { return m_w; }
    int Height() const { return m_h; }

private:
    SDL_Texture* m_tex = nullptr;
    int m_w = 0;
    int m_h = 0;
};
"""

TEXTURE_CPP = r"""#include "gfx/Texture.h"
#include "core/Log.h"
#include <SDL.h>

Texture::~Texture()
{
    Destroy();
}

void Texture::Destroy()
{
    if (m_tex)
    {
        SDL_DestroyTexture(m_tex);
        m_tex = nullptr;
    }
    m_w = 0; m_h = 0;
}

bool Texture::LoadBMP(SDL_Renderer* r, const std::string& path, bool colorKeyBlack)
{
    Destroy();

    SDL_Surface* surf = SDL_LoadBMP(path.c_str());
    if (!surf)
    {
        logx::Error(std::string("SDL_LoadBMP failed for ") + path + ": " + SDL_GetError());
        return false;
    }

    if (colorKeyBlack)
    {
        // treat black as transparent
        Uint32 key = SDL_MapRGB(surf->format, 0, 0, 0);
        SDL_SetColorKey(surf, SDL_TRUE, key);
    }

    m_tex = SDL_CreateTextureFromSurface(r, surf);
    if (!m_tex)
    {
        logx::Error(std::string("SDL_CreateTextureFromSurface failed: ") + SDL_GetError());
        SDL_FreeSurface(surf);
        return false;
    }

    m_w = surf->w;
    m_h = surf->h;
    SDL_FreeSurface(surf);

    return true;
}
"""

RENDERER_H = r"""#pragma once
#include "gfx/Color.h"

struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Rect;

class Renderer
{
public:
    explicit Renderer(SDL_Renderer* r);

    SDL_Renderer* Raw() const { return m_r; }

    void Clear();
    void Present();

    void FillRect(int x, int y, int w, int h, Color c);
    void DrawRect(int x, int y, int w, int h, Color c);

    void Blit(SDL_Texture* tex, const SDL_Rect& src, const SDL_Rect& dst);

private:
    SDL_Renderer* m_r = nullptr;
};
"""

RENDERER_CPP = r"""#include "gfx/Renderer.h"
#include <SDL.h>

Renderer::Renderer(SDL_Renderer* r) : m_r(r) {}

void Renderer::Clear()
{
    SDL_SetRenderDrawColor(m_r, 0, 0, 0, 255);
    SDL_RenderClear(m_r);
}

void Renderer::Present()
{
    SDL_RenderPresent(m_r);
}

void Renderer::FillRect(int x, int y, int w, int h, Color c)
{
    SDL_Rect rc{ x, y, w, h };
    SDL_SetRenderDrawColor(m_r, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(m_r, &rc);
}

void Renderer::DrawRect(int x, int y, int w, int h, Color c)
{
    SDL_Rect rc{ x, y, w, h };
    SDL_SetRenderDrawColor(m_r, c.r, c.g, c.b, c.a);
    SDL_RenderDrawRect(m_r, &rc);
}

void Renderer::Blit(SDL_Texture* tex, const SDL_Rect& src, const SDL_Rect& dst)
{
    SDL_RenderCopy(m_r, tex, &src, &dst);
}
"""

FONT_H = r"""#pragma once
#include <string>
#include "gfx/Texture.h"

class Renderer;

class Font
{
public:
    explicit Font(Renderer& r);
    ~Font() = default;

    bool LoadAtlasBMP(const std::string& path, int glyphW, int glyphH);

    // Text is drawn in screen pixels.
    void DrawText(Renderer& r, int x, int y, const std::string& text);

    int GlyphW() const { return m_glyphW; }
    int GlyphH() const { return m_glyphH; }

private:
    Texture m_atlas;
    int m_glyphW = 16;
    int m_glyphH = 16;
    int m_cols = 16;
};

"""

FONT_CPP = r"""#include "gfx/Font.h"
#include "gfx/Renderer.h"
#include "core/Log.h"
#include <SDL.h>

Font::Font(Renderer& r)
{
    (void)r;
}

bool Font::LoadAtlasBMP(const std::string& path, int glyphW, int glyphH)
{
    m_glyphW = glyphW;
    m_glyphH = glyphH;

    // black = transparent
    if (!m_atlas.LoadBMP(nullptr, path, true))
    {
        // The Texture needs a renderer; LoadBMP uses SDL_CreateTextureFromSurface
        // so we must load via the Renderer later. We'll do a corrected load below.
        return false;
    }
    return true;
}

// Corrected: we need renderer to create texture. We'll do lazy init approach:
static bool LoadTextureWithRenderer(Texture& t, Renderer& r, const std::string& path)
{
    return t.LoadBMP(r.Raw(), path, true);
}

void Font::DrawText(Renderer& r, int x, int y, const std::string& text)
{
    // If atlas not loaded yet (or loaded incorrectly), try loading now
    if (!m_atlas.Get())
    {
        // default asset path
        const std::string path = "assets/fonts/font16x16.bmp";
        if (!LoadTextureWithRenderer(m_atlas, r, path))
        {
            // If font fails, draw nothing (fail-safe).
            return;
        }
    }

    const int atlasW = m_atlas.Width();
    if (atlasW <= 0) return;

    m_cols = atlasW / m_glyphW;
    if (m_cols <= 0) m_cols = 16;

    int penX = x;
    int penY = y;

    for (unsigned char ch : text)
    {
        if (ch == '\n')
        {
            penX = x;
            penY += m_glyphH;
            continue;
        }

        const int idx = static_cast<int>(ch);
        const int sx = (idx % m_cols) * m_glyphW;
        const int sy = (idx / m_cols) * m_glyphH;

        SDL_Rect src{ sx, sy, m_glyphW, m_glyphH };
        SDL_Rect dst{ penX, penY, m_glyphW, m_glyphH };
        r.Blit(m_atlas.Get(), src, dst);

        penX += m_glyphW;
    }
}
"""

INPUT_H = r"""#pragma once
#include <SDL.h>
#include <unordered_map>

class Input
{
public:
    void BeginFrame();
    void ProcessEvent(const SDL_Event& e);

    bool Down(SDL_Keycode key) const;
    bool PressedOnce(SDL_Keycode key) const;

private:
    std::unordered_map<SDL_Keycode, bool> m_down;
    std::unordered_map<SDL_Keycode, bool> m_pressedThisFrame;
};
"""

INPUT_CPP = r"""#include "input/Input.h"

void Input::BeginFrame()
{
    m_pressedThisFrame.clear();
}

void Input::ProcessEvent(const SDL_Event& e)
{
    if (e.type == SDL_KEYDOWN && !e.key.repeat)
    {
        m_down[e.key.keysym.sym] = true;
        m_pressedThisFrame[e.key.keysym.sym] = true;
    }
    else if (e.type == SDL_KEYUP)
    {
        m_down[e.key.keysym.sym] = false;
    }
}

bool Input::Down(SDL_Keycode key) const
{
    auto it = m_down.find(key);
    return (it != m_down.end()) ? it->second : false;
}

bool Input::PressedOnce(SDL_Keycode key) const
{
    auto it = m_pressedThisFrame.find(key);
    return (it != m_pressedThisFrame.end()) ? it->second : false;
}
"""

UI_H = r"""#pragma once
#include <string>

class Font;
class Renderer;

class Ui
{
public:
    explicit Ui(Font& font);

    void ToggleMenu();
    bool IsMenuOpen() const { return m_menuOpen; }

    void Tick();
    void Render(Renderer& r);

private:
    Font& m_font;
    bool m_menuOpen = false;

    // later: focus, buttons, menus, etc.
};
"""

UI_CPP = r"""#include "ui/Ui.h"
#include "gfx/Font.h"
#include "gfx/Renderer.h"
#include "gfx/Color.h"
#include "core/Config.h"

Ui::Ui(Font& font) : m_font(font) {}

void Ui::ToggleMenu()
{
    m_menuOpen = !m_menuOpen;
}

void Ui::Tick()
{
    // placeholder for UI logic (selection, etc.)
}

void Ui::Render(Renderer& r)
{
    if (!m_menuOpen) return;

    // Dim background overlay
    r.FillRect(0, 0, cfg::WindowWidth, cfg::WindowHeight, Color::RGBA(0, 0, 0, 180));

    // Menu panel
    const int w = 520;
    const int h = 260;
    const int x = (cfg::WindowWidth - w) / 2;
    const int y = (cfg::WindowHeight - h) / 2;

    r.FillRect(x, y, w, h, Color::RGBA(20, 20, 20, 240));
    r.DrawRect(x, y, w, h, Color::RGB(200, 200, 200));

    int tx = x + 24;
    int ty = y + 24;
    m_font.DrawText(r, tx, ty, "MENU");
    ty += m_font.GlyphH() * 2;

    m_font.DrawText(r, tx, ty, "ESC - Close menu"); ty += m_font.GlyphH();
    m_font.DrawText(r, tx, ty, "Q   - Quit");       ty += m_font.GlyphH();

    ty += m_font.GlyphH();
    m_font.DrawText(r, tx, ty, "Next steps:");
    ty += m_font.GlyphH();
    m_font.DrawText(r, tx, ty, "- Build dungeon core simulation");
    ty += m_font.GlyphH();
    m_font.DrawText(r, tx, ty, "- Add spawners + invader waves");
}
"""

TILES_H = r"""#pragma once
#include <cstdint>

enum class TileType : uint8_t
{
    Rock,
    Floor,
    Wall,
    Core,
    Spawner,
};

struct Tile
{
    TileType type = TileType::Rock;
};
"""

WORLD_H = r"""#pragma once
#include "world/Tiles.h"

class Renderer;
class Font;

class World
{
public:
    World();
    void Tick();

    void Render(Renderer& r, Font& font, int camX, int camY);

private:
    Tile m_tiles[80 * 45]; // matches cfg::WorldW/H, kept simple here
};
"""

WORLD_CPP = r"""#include "world/World.h"
#include "core/Config.h"
#include "gfx/Renderer.h"
#include "gfx/Font.h"
#include "gfx/Color.h"
#include <string>

static Color TileColor(TileType t)
{
    switch (t)
    {
        case TileType::Rock:    return Color::RGB(30, 30, 30);
        case TileType::Floor:   return Color::RGB(60, 60, 60);
        case TileType::Wall:    return Color::RGB(120, 120, 120);
        case TileType::Core:    return Color::RGB(120, 40, 180);
        case TileType::Spawner: return Color::RGB(40, 180, 120);
    }
    return Color::RGB(255, 0, 255);
}

static char TileGlyph(TileType t)
{
    switch (t)
    {
        case TileType::Rock:    return '#';
        case TileType::Floor:   return '.';
        case TileType::Wall:    return 'W';
        case TileType::Core:    return 'C';
        case TileType::Spawner: return 'S';
    }
    return '?';
}

World::World()
{
    // Initialize a simple dungeon-like layout
    for (int y = 0; y < cfg::WorldH; ++y)
    {
        for (int x = 0; x < cfg::WorldW; ++x)
        {
            Tile& t = m_tiles[y * cfg::WorldW + x];
            t.type = TileType::Rock;
        }
    }

    // Carve a rectangular room
    for (int y = 10; y < 35; ++y)
    {
        for (int x = 12; x < 68; ++x)
        {
            Tile& t = m_tiles[y * cfg::WorldW + x];
            const bool border = (x == 12 || x == 67 || y == 10 || y == 34);
            t.type = border ? TileType::Wall : TileType::Floor;
        }
    }

    // Place core + spawners
    m_tiles[22 * cfg::WorldW + 40].type = TileType::Core;
    m_tiles[18 * cfg::WorldW + 25].type = TileType::Spawner;
    m_tiles[18 * cfg::WorldW + 55].type = TileType::Spawner;
    m_tiles[30 * cfg::WorldW + 25].type = TileType::Spawner;
    m_tiles[30 * cfg::WorldW + 55].type = TileType::Spawner;
}

void World::Tick()
{
    // placeholder: later this advances simulation:
    // - power generation / mana
    // - spawner cooldowns
    // - invader pathing
}

void World::Render(Renderer& r, Font& font, int camX, int camY)
{
    // Render a camera-sized view; for now, draw from (0,0) and clamp in App.
    const int tilesAcross = cfg::WindowWidth / cfg::TileSizePx;
    const int tilesDown   = cfg::WindowHeight / cfg::TileSizePx;

    for (int sy = 0; sy < tilesDown; ++sy)
    {
        for (int sx = 0; sx < tilesAcross; ++sx)
        {
            const int wx = camX + sx;
            const int wy = camY + sy;
            if (wx < 0 || wy < 0 || wx >= cfg::WorldW || wy >= cfg::WorldH)
                continue;

            const Tile& t = m_tiles[wy * cfg::WorldW + wx];
            const int px = sx * cfg::TileSizePx;
            const int py = sy * cfg::TileSizePx;

            r.FillRect(px, py, cfg::TileSizePx, cfg::TileSizePx, TileColor(t.type));

            // Glyph overlay (text-mode feel)
            char s[2] = { TileGlyph(t.type), 0 };
            font.DrawText(r, px, py, s);
        }
    }

    // Debug readout
    font.DrawText(r, 8, cfg::WindowHeight - 24, "World: room + core + 4 spawners (prototype)");
}
"""

CORE_H = r"""#pragma once

class DungeonCore
{
public:
    // placeholder: later holds mana, upgrades, influence radius, etc.
    void Tick() {}
};
"""

CORE_CPP = r"""#include "world/DungeonCore.h"
"""

if __name__ == "__main__":
    main()
