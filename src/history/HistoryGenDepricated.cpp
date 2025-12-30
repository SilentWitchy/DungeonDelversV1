#include "history/HistoryGen.h"
#include <algorithm>

HistoryGen::HistoryGen(uint32_t seed) : m_seed(seed) {}

uint32_t HistoryGen::Next(uint32_t& state) const
{
    // xorshift32
    uint32_t x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state = x;
    return x;
}

int HistoryGen::RandRange(uint32_t& state, int lo, int hi) const
{
    if (hi <= lo) return lo;
    const uint32_t r = Next(state);
    const int span = (hi - lo + 1);
    return lo + (int)(r % (uint32_t)span);
}

float HistoryGen::Volatility01(const WorldGenSettings& s) const
{
    // stable..chaotic : 0..4
    // map to 0..1
    return std::clamp(s.worldVolatility / 4.0f, 0.0f, 1.0f);
}

std::string HistoryGen::Pick(const std::vector<std::string>& v, uint32_t& state) const
{
    if (v.empty()) return "UNKNOWN";
    const int i = RandRange(state, 0, (int)v.size() - 1);
    return v[i];
}

std::vector<std::string> HistoryGen::GenerateEvents(const WorldGenSettings& s) const
{
    uint32_t st = (m_seed == 0) ? 1u : m_seed;

    // how many events: influenced by history length 0..4
    const int base = 6;
    const int count = base + s.historyLength * 4; // 6..22

    // simple component pools for now (these move to .txt next)
    std::vector<std::string> concepts = { "Ash", "Flame", "Silence", "Depths", "Faith", "Storm" };
    std::vector<std::string> descriptors = { "Black", "Fallen", "Sacred", "Forgotten", "Eternal" };

    // volatility biases more war-like events
    const float v = Volatility01(s);

    // templates split into peaceful vs violent sets (temporary)
    std::vector<std::string> peaceful = {
        "The Age of {concept}",
        "The {descriptor} Age",
        "The Treaty of {concept}",
        "The Founding of {descriptor} Haven"
    };
    std::vector<std::string> violent = {
        "The {descriptor} War",
        "The War of {concept}",
        "The {descriptor} Crusade",
        "The Battle of {descriptor} Ford",
        "The Collapse of the {descriptor} Order"
    };

    auto resolve = [&](std::string t) -> std::string
        {
            // very small placeholder replacement
            auto rep = [&](const char* key, const std::string& val)
                {
                    const std::string k = key;
                    size_t pos = 0;
                    while ((pos = t.find(k, pos)) != std::string::npos)
                    {
                        t.replace(pos, k.size(), val);
                        pos += val.size();
                    }
                };

            rep("{concept}", Pick(concepts, st));
            rep("{descriptor}", Pick(descriptors, st));
            return t;
        };

    std::vector<std::string> out;
    out.reserve(count);

    int year = 1;
    for (int i = 0; i < count; ++i)
    {
        // years advance more when history length is longer
        year += RandRange(st, 1, 6) + s.historyLength;

        // choose violent with probability based on volatility
        const int roll = RandRange(st, 0, 100);
        const bool pickViolent = (roll < (int)(20 + 70 * v)); // ~20%..90%

        std::string templ = pickViolent ? Pick(violent, st) : Pick(peaceful, st);
        out.push_back("Year " + std::to_string(year) + ": " + resolve(templ));
    }

    return out;
}
