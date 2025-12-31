#pragma once
#include <SDL.h>
#include <unordered_map>

class Input
{
public:
    void BeginFrame();
    void ProcessEvent(const SDL_Event& e);

    bool Down(SDL_Keycode key) const;
    bool PressedOnce(SDL_Keycode key) const;
    int  WheelY() const { return m_wheelY; }

private:
    std::unordered_map<SDL_Keycode, bool> m_down;
    std::unordered_map<SDL_Keycode, bool> m_pressedThisFrame;
    int m_wheelY = 0;
};
