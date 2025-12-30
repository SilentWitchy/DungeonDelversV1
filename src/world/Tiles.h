#pragma once
#include <cstdint>
#include "gfx/Color.h"

// =======================================================
// Tile types
// =======================================================

enum class TileType : uint8_t
{
    // Water
    Ocean,
    Coast,
    Lake,
    River,

    // Land / biomes
    Plains,
    Forest,
    Jungle,
    Desert,
    Tundra,
    Hill,
    Mountain,

    // World features
    City,
    Ruin,
    DungeonSite,
    Artifact,

    // Dungeon tiles
    Rock,
    Floor,
    Wall,
    Core,
    Spawner,
};

// =======================================================
// Tile data
// =======================================================

struct Tile
{
    TileType type = TileType::Ocean;

    float elevation = 0.0f;
    float temperature = 0.0f;
    float moisture = 0.0f;
    float mineralRichness = 0.0f;
    float vegetationDensity = 0.0f;

    bool hasRiver = false;
    bool isLake = false;
    bool isStartingPoint = false;
};

// =======================================================
// Glyph mapping (ASCII rendering)
// =======================================================

inline char TileGlyph(const Tile& tile)
{
    if (tile.isStartingPoint)
        return 'P';

    switch (tile.type)
    {
    case TileType::Ocean:       return '~';
    case TileType::Coast:       return ',';
    case TileType::Lake:        return 'o';
    case TileType::River:       return '=';

    case TileType::Plains:      return '.';
    case TileType::Forest:      return 'Y';
    case TileType::Jungle:      return 'J';
    case TileType::Desert:      return ':';
    case TileType::Tundra:      return '"';
    case TileType::Hill:        return 'n';
    case TileType::Mountain:    return '^';

    case TileType::City:        return 'T';
    case TileType::Ruin:        return 'R';
    case TileType::DungeonSite: return 'D';
    case TileType::Artifact:    return 'A';

    case TileType::Rock:        return '#';
    case TileType::Wall:        return '#';
    case TileType::Floor:       return '.';
    case TileType::Core:        return 'C';
    case TileType::Spawner:     return 'S';

    default:                    return '?';
    }
}

// =======================================================
// Color mapping (visual rendering)
// =======================================================

inline Color TileColor(const Tile& tile)
{
    switch (tile.type)
    {
    case TileType::Ocean:       return Color(20, 70, 150);
    case TileType::Coast:       return Color(200, 190, 140);
    case TileType::Lake:        return Color(40, 100, 190);
    case TileType::River:       return Color(60, 130, 210);

    case TileType::Plains:      return Color(90, 170, 90);
    case TileType::Forest:      return Color(40, 120, 70);
    case TileType::Jungle:      return Color(50, 150, 80);
    case TileType::Desert:      return Color(220, 190, 100);
    case TileType::Tundra:      return Color(200, 220, 230);
    case TileType::Hill:        return Color(130, 150, 90);
    case TileType::Mountain:    return Color(190, 190, 190);

    case TileType::City:        return Color(80, 140, 220);
    case TileType::Ruin:        return Color(120, 120, 120);
    case TileType::DungeonSite: return Color(180, 80, 80);
    case TileType::Artifact:    return Color(240, 200, 60);

    case TileType::Rock:        return Color(90, 90, 90);
    case TileType::Wall:        return Color(110, 110, 110);
    case TileType::Floor:       return Color(140, 140, 140);
    case TileType::Core:        return Color(160, 60, 200);
    case TileType::Spawner:     return Color(120, 200, 120);

    default:                    return Color(255, 0, 255);
    }
}
