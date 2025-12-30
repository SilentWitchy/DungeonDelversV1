#include "gfx/Renderer.h"
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
