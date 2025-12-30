#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "world/WorldGenSettings.h"

struct WorldEntity
{
    uint32_t id = 0;
    std::string type;                 // "city", "kingdom", "dungeon", "artifact", "continent", "ocean", "org", ...
    std::string name;
    std::vector<std::string> tags;    // lowercase

    // gameplay flags (consumed by mapgen/loot later)
    bool spawnCandidate = false;      // should try to exist on the map now
    bool ruinCandidate = false;      // can appear as ruin/remnant
    bool itemSpawnable = false;      // can appear in loot tables
};

struct SpawnIntent
{
    uint32_t entityId = 0;
    std::string spawnType;            // "place_city", "place_ruin", "seed_artifact", ...
    float probability = 0.0f;         // 0..1
};

struct HistoricalEvent
{
    int year = 0;
    std::string category;             // lowercase: "wars", "falls", ...
    std::string title;                // final rendered string

    std::vector<uint32_t> involvedEntityIds;
    std::vector<SpawnIntent> spawnIntents;
};

struct WorldHistoryPackage
{
    uint32_t seed = 0;
    WorldGenSettings settings{};

    std::vector<WorldEntity> entities;
    std::vector<HistoricalEvent> events;

    // convenience lists for later mapgen consumption
    std::vector<uint32_t> citiesToPlace;
    std::vector<uint32_t> ruinsToPlace;
    std::vector<uint32_t> kingdomsToRepresent;
    std::vector<uint32_t> artifactsToSeed;

    // helper for UI display
    std::vector<std::string> ToDisplayLines() const
    {
        std::vector<std::string> out;
        out.reserve(events.size());
        for (auto& e : events)
            out.push_back("Year " + std::to_string(e.year) + ": " + e.title);
        return out;
    }
};
