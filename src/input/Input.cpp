#include "input/Input.h"

void Input::BeginFrame()
{
    m_pressedThisFrame.clear();
    m_wheelY = 0;
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
    else if (e.type == SDL_MOUSEWHEEL)
    {
        m_wheelY += e.wheel.y;
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
