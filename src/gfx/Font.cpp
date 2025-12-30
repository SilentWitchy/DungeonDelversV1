#include "gfx/Font.h"
#include "gfx/Renderer.h"
#include "core/Log.h"
#include <SDL.h>

Font::Font(Renderer& r)
{
    (void)r;
}

bool Font::LoadAtlasBMP(Renderer& r, const std::string& path, int glyphW, int glyphH)
{
    m_glyphW = glyphW;
    m_glyphH = glyphH;

    if (!m_atlas.LoadBMP(r.Raw(), path, true))
    {
        logx::Error(std::string("Failed to load font atlas: ") + path);
        return false;
    }

    const int atlasW = m_atlas.Width();
    if (atlasW <= 0 || (atlasW % m_glyphW) != 0)
    {
        logx::Warn("Font atlas width is not divisible by glyph width; check your atlas image.");
    }

    m_cols = std::max(1, atlasW / m_glyphW);
    return true;
}

void Font::DrawText(Renderer& r, int x, int y, const std::string& text)
{
    if (!m_atlas.Get())
        return; // font not loaded; fail-safe

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
