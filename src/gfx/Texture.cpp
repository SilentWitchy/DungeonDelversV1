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
