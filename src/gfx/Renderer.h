#pragma once
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
