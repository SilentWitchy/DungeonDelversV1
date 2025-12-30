#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "history/HistoryData.h"
#include "history/WorldHistoryPackage.h"
#include "world/WorldGenSettings.h"

class WorldHistoryGen
{
public:
    WorldHistoryGen(uint32_t seed, const HistoryData& data);
    WorldHistoryPackage Generate(const WorldGenSettings& settings);

private:
    uint32_t Next(uint32_t& state) const;
    int RandRange(uint32_t& state, int lo, int hi) const;
    float Rand01(uint32_t& state) const;

    float Volatility01(const WorldGenSettings& s) const;

    std::string PickCategory(uint32_t& rng, const WorldGenSettings& s) const;
    const EventTemplate* PickTemplateFromCategory(uint32_t& rng, const std::string& category) const;

    bool PickNameByTags(const std::vector<std::string>& requiredTags, std::string& outName) const;

    uint32_t GetOrCreateEntity(const std::string& type,
        const std::string& name,
        const std::vector<std::string>& tags);

    std::string ResolvePattern(uint32_t& rng,
        const std::string& category,
        const std::string& pattern,
        std::vector<uint32_t>& outInvolvedEntityIds);

    void AddSpawnIntents(const WorldGenSettings& s,
        const std::string& category,
        const std::vector<uint32_t>& involved,
        std::vector<SpawnIntent>& outIntents);

    void BuildConvenienceLists(WorldHistoryPackage& pkg);

private:
    uint32_t m_seed = 1;
    const HistoryData& m_data;

    WorldHistoryPackage* m_pkg = nullptr;
    std::unordered_map<std::string, uint32_t> m_entityIndex;
};
