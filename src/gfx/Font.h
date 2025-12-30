#pragma once
#include <string>
#include "gfx/Texture.h"

class Renderer;

class Font
{
public:
    explicit Font(Renderer& r);
    ~Font() = default;

    bool LoadAtlasBMP(Renderer& r, const std::string& path, int glyphW, int glyphH);

    void DrawText(Renderer& r, int x, int y, const std::string& text);

    int GlyphW() const { return m_glyphW; }
    int GlyphH() const { return m_glyphH; }

private:
    Texture m_atlas;
    int m_glyphW = 16;
    int m_glyphH = 16;
    int m_cols = 16;
};
