#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "world/World.h" // for WorldGenSettings

class HistoryGen
{
public:
    explicit HistoryGen(uint32_t seed);

    // Returns event strings like: "Year 12: The Age of Ash"
    std::vector<std::string> GenerateEvents(const WorldGenSettings& s) const;

private:
    uint32_t m_seed = 0;

    // tiny deterministic RNG
    uint32_t Next(uint32_t& state) const;
    int RandRange(uint32_t& state, int lo, int hi) const;

    float Volatility01(const WorldGenSettings& s) const;

    std::string Pick(const std::vector<std::string>& v, uint32_t& state) const;
};
