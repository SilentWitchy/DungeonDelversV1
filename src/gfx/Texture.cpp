#include "gfx/Texture.h"
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
    m_w = 0;
    m_h = 0;
    m_streaming = false;
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
    m_streaming = false;

    SDL_FreeSurface(surf);
    return true;
}

bool Texture::LoadFromPixels(SDL_Renderer* r, const uint32_t* pixels, int w, int h)
{
    Destroy();

    SDL_Texture* tex = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, w, h);
    if (!tex)
    {
        logx::Error(std::string("SDL_CreateTexture failed: ") + SDL_GetError());
        return false;
    }

    const int pitch = w * static_cast<int>(sizeof(uint32_t));
    if (SDL_UpdateTexture(tex, nullptr, pixels, pitch) != 0)
    {
        logx::Error(std::string("SDL_UpdateTexture failed: ") + SDL_GetError());
        SDL_DestroyTexture(tex);
        return false;
    }

    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_NONE);

    m_tex = tex;
    m_w = w;
    m_h = h;
    m_streaming = false;
    return true;
}

// -------------------------
// NEW: Streaming support
// -------------------------

bool Texture::CreateRGBAStreaming(SDL_Renderer* r, int w, int h)
{
    Destroy();

    SDL_Texture* tex = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, w, h);
    if (!tex)
    {
        logx::Error(std::string("SDL_CreateTexture (streaming) failed: ") + SDL_GetError());
        return false;
    }

    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_NONE);

    m_tex = tex;
    m_w = w;
    m_h = h;
    m_streaming = true;
    return true;
}

bool Texture::UpdateRGBA(const void* pixelsRGBA8888, int pitchBytes)
{
    if (!m_tex)
    {
        logx::Error("UpdateRGBA called on null texture");
        return false;
    }

    if (SDL_UpdateTexture(m_tex, nullptr, pixelsRGBA8888, pitchBytes) != 0)
    {
        logx::Error(std::string("SDL_UpdateTexture failed: ") + SDL_GetError());
        return false;
    }

    return true;
}
