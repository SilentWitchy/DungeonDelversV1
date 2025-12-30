#ifndef DUNGEONDELVS_GFX_TEXTURE_H
#define DUNGEONDELVS_GFX_TEXTURE_H

#include <cstdint>
#include <string>

struct SDL_Texture;
struct SDL_Renderer;

class Texture
{
public:
    Texture() = default;
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    bool LoadBMP(SDL_Renderer* r, const std::string& path, bool colorKeyBlack);
    bool LoadFromPixels(SDL_Renderer* r, const uint32_t* pixels, int w, int h);

    // Streaming (for dynamic pixel updates)
    bool CreateRGBAStreaming(SDL_Renderer* r, int w, int h);
    bool UpdateRGBA(const void* pixelsRGBA8888, int pitchBytes);

    void Destroy();

    SDL_Texture* Get() const { return m_tex; }
    int Width() const { return m_w; }
    int Height() const { return m_h; }

private:
    SDL_Texture* m_tex = nullptr;
    int m_w = 0;
    int m_h = 0;
    bool m_streaming = false;
};

#endif
