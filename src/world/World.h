#pragma once
#include "world/Tiles.h"
#include "world/WorldGenSettings.h"
#include "history/WorldHistoryPackage.h"
#include <utility>

class Renderer;
class Font;

class World
{
public:
    World();
    World(const WorldGenSettings& settings);
    World(const WorldGenSettings& settings, const WorldHistoryPackage& history);
    void Tick();
    void Render(Renderer& r, Font& font, int camX, int camY);
    void RenderFullMap(Renderer& r, Font& font, int camX, int camY);
    std::pair<int, int> GetStartPosition() const { return m_startPos; }

    int StartX() const { return m_startX; }
    int StartY() const { return m_startY; }

private:
    void Generate(const WorldGenSettings& settings);
    void ApplyHistorySpawns(const WorldHistoryPackage& history);

private:
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    int m_startX = 0;
    int m_startY = 0;
    Tile m_tiles[80 * 45];
    std::pair<int, int> m_startPos{0, 0};
};
