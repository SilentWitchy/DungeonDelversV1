#pragma once
#include <cstdint>
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
    bool LoadFromPixels(SDL_Renderer* r, const uint32_t* pixels, int w, int h);
    void Destroy();

    SDL_Texture* Get() const { return m_tex; }
    int Width() const { return m_w; }
    int Height() const { return m_h; }

private:
    SDL_Texture* m_tex = nullptr;
    int m_w = 0;
    int m_h = 0;
};
