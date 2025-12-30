#include "history/WorldHistoryGen.h"
#include <algorithm>
#include <cctype>

static std::string ToLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

// src/history/WorldHistoryGen.cpp

static float BaseCategoryWeight(const std::string& cat)
{
    // baseline “how common is this category” before settings
    if (cat == "ages")        return 1.0f;
    if (cat == "revolutions") return 0.8f;
    if (cat == "wars")        return 0.7f;
    if (cat == "crusades")    return 0.4f;
    if (cat == "battles")     return 0.6f;
    if (cat == "falls")       return 0.5f;
    if (cat == "foundings")   return 0.7f;
    if (cat == "political")   return 0.8f;
    if (cat == "crises")      return 0.6f;
    return 0.5f;
}

static float VolatilityMultiplier(const std::string& cat, float v01)
{
    // v01 in [0,1]. Increase violent categories as volatility rises.
    if (cat == "wars" || cat == "battles" || cat == "crusades") return 1.0f + 3.0f * v01;
    if (cat == "falls" || cat == "crises")                      return 1.0f + 2.0f * v01;
    if (cat == "political")                                     return 1.0f - 0.4f * v01;
    if (cat == "ages")                                          return 1.0f - 0.3f * v01;
    return 1.0f;
}


static std::vector<std::string> SplitTagsUnderscore(const std::string& s)
{
    std::vector<std::string> out;
    std::string cur;
    for (char c : s)
    {
        if (c == '_')
        {
            if (!cur.empty()) out.push_back(ToLower(cur));
            cur.clear();
        }
        else
        {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) out.push_back(ToLower(cur));
    return out;
}

WorldHistoryGen::WorldHistoryGen(uint32_t seed, const HistoryData& data)
    : m_seed(seed ? seed : 1u), m_data(data)
{
}

uint32_t WorldHistoryGen::Next(uint32_t& state) const
{
    // xorshift32
    uint32_t x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state = x;
    return x;
}

int WorldHistoryGen::RandRange(uint32_t& state, int lo, int hi) const
{
    if (hi <= lo) return lo;
    const uint32_t r = Next(state);
    const int span = (hi - lo + 1);
    return lo + (int)(r % (uint32_t)span);
}

float WorldHistoryGen::Rand01(uint32_t& state) const
{
    const uint32_t r = Next(state);
    return (float)(r / 4294967295.0);
}

float WorldHistoryGen::Volatility01(const WorldGenSettings& s) const
{
    return std::clamp(s.worldVolatility / 4.0f, 0.0f, 1.0f); // 0..4 -> 0..1
}

uint32_t WorldHistoryGen::GetOrCreateEntity(const std::string& type,
    const std::string& name,
    const std::vector<std::string>& tags)
{
    const std::string key = type + "\n" + name;
    auto it = m_entityIndex.find(key);
    if (it != m_entityIndex.end())
        return it->second;

    WorldEntity e;
    e.id = (uint32_t)(m_pkg->entities.size() + 1);
    e.type = type;
    e.name = name;
    e.tags = tags;

    m_pkg->entities.push_back(e);
    m_entityIndex[key] = e.id;
    return e.id;
}

bool WorldHistoryGen::PickNameByTags(const std::vector<std::string>& requiredTags, std::string& outName) const
{
    if (m_data.names.empty())
        return false;

    auto hasAll = [&](const NameEntry& e) -> bool
        {
            for (const auto& rt : requiredTags)
            {
                bool found = false;
                for (const auto& tag : e.tags)
                {
                    if (tag == rt) { found = true; break; }
                }
                if (!found) return false;
            }
            return true;
        };

    for (const auto& e : m_data.names)
    {
        if (hasAll(e))
        {
            outName = e.text;
            return true;
        }
    }
    return false;
}

// src/history/WorldHistoryGen.cpp

std::string WorldHistoryGen::PickCategory(uint32_t& rng, const WorldGenSettings& s) const
{
    const float v = Volatility01(s);

    struct WCat { std::string cat; float w; };
    std::vector<WCat> cats;
    cats.reserve(m_data.templates.size());

    float total = 0.0f;
    for (const auto& kv : m_data.templates)
    {
        if (kv.second.empty()) continue;

        const std::string& cat = kv.first; // already lowercase from loader
        float w = BaseCategoryWeight(cat) * VolatilityMultiplier(cat, v);

        // Optional: history length can also increase total violent events a bit
        // (kept mild, so volatility remains the main lever)
        w *= (1.0f + 0.05f * (float)s.historyLength);

        if (w < 0.01f) w = 0.01f;
        cats.push_back({ cat, w });
        total += w;
    }

    if (cats.empty())
        return "wars";

    float roll = Rand01(rng) * total;
    float acc = 0.0f;
    for (const auto& c : cats)
    {
        acc += c.w;
        if (roll <= acc)
            return c.cat;
    }
    return cats.back().cat;
}
const EventTemplate* WorldHistoryGen::PickTemplateFromCategory(uint32_t& rng, const std::string& category) const
{
    auto it = m_data.templates.find(category);
    if (it == m_data.templates.end() || it->second.empty())
        return nullptr;

    const int idx = RandRange(rng, 0, (int)it->second.size() - 1);
    return &it->second[idx];
}


std::string WorldHistoryGen::ResolvePattern(uint32_t& rng,
    const std::string& category,
    const std::string& pattern,
    std::vector<uint32_t>& outInvolvedEntityIds)
{
    std::string out;
    out.reserve(pattern.size() + 32);

    for (size_t i = 0; i < pattern.size(); )
    {
        if (pattern[i] == '{')
        {
            size_t j = pattern.find('}', i + 1);
            if (j == std::string::npos)
            {
                out.push_back(pattern[i++]);
                continue;
            }

            const std::string placeholderRaw = pattern.substr(i + 1, j - (i + 1));
            const std::string placeholder = ToLower(placeholderRaw);

            // pool-backed placeholders
            if (placeholder == "concept" || placeholder == "descriptor" || placeholder == "group")
            {
                std::string poolKey = (placeholder == "concept") ? "concepts"
                    : (placeholder == "descriptor") ? "descriptors"
                    : "groups";

                auto pit = m_data.pools.find(poolKey);
                if (pit != m_data.pools.end() && !pit->second.empty())
                {
                    const int idx = RandRange(rng, 0, (int)pit->second.size() - 1);
                    out += pit->second[idx];
                }
                else
                {
                    out += "UNKNOWN";
                }

                i = j + 1;
                continue;
            }

            // tag-backed placeholder: split by underscore into required tags
            const std::vector<std::string> tags = SplitTagsUnderscore(placeholder);

            // pick a matching name
            // collect all matching entries
            std::vector<const NameEntry*> matches;
            for (const auto& e : m_data.names)
            {
                bool ok = true;
                for (const auto& req : tags)
                {
                    bool has = false;
                    for (const auto& t : e.tags)
                    {
                        if (t == req) { has = true; break; }
                    }
                    if (!has) { ok = false; break; }
                }
                if (ok) matches.push_back(&e);
            }

            if (matches.empty())
            {
                out += "UNKNOWN";
                i = j + 1;
                continue;
            }

            const int pick = RandRange(rng, 0, (int)matches.size() - 1);
            const NameEntry* chosen = matches[pick];

            // Determine entity types for certain tags (so events can reference real objects)
            // If none match, treat as plain text.
            std::string entityType;
            auto hasTag = [&](const char* t) {
                for (const auto& x : chosen->tags) if (x == t) return true;
                return false;
                };

            if (hasTag("city")) entityType = "city";
            else if (hasTag("kingdom")) entityType = "kingdom";
            else if (hasTag("ocean")) entityType = "ocean";
            else if (hasTag("continent")) entityType = "continent";
            else if (hasTag("dungeon")) entityType = "dungeon";
            else if (hasTag("legendaryitem")) entityType = "artifact";
            else if (hasTag("legendarycreature")) entityType = "legendarycreature";
            else if (hasTag("organization") || hasTag("org")) entityType = "org";

            if (!entityType.empty())
            {
                const uint32_t id = GetOrCreateEntity(entityType, chosen->text, chosen->tags);
                outInvolvedEntityIds.push_back(id);
            }

            out += chosen->text;
            i = j + 1;
            continue;
        }

        out.push_back(pattern[i++]);
    }

    // optional: if category suggests a city/kingdom but template didn't contain one,
    // we still keep it text-only for now.

    return out;
}

void WorldHistoryGen::AddSpawnIntents(const WorldGenSettings& s,
    const std::string& category,
    const std::vector<uint32_t>& involved,
    std::vector<SpawnIntent>& outIntents)
{
    const float v = Volatility01(s);

    auto add = [&](uint32_t id, const char* type, float p)
        {
            SpawnIntent si;
            si.entityId = id;
            si.spawnType = type;
            si.probability = std::clamp(p, 0.0f, 1.0f);
            outIntents.push_back(si);
        };

    // heuristics (first pass)
    for (uint32_t id : involved)
    {
        const WorldEntity* e = nullptr;
        for (auto& ent : m_pkg->entities)
            if (ent.id == id) { e = &ent; break; }
        if (!e) continue;

        if (category == "foundings")
        {
            if (e->type == "city")   add(id, "place_city", 0.90f);
            if (e->type == "kingdom") add(id, "represent_kingdom", 0.85f);
        }
        else if (category == "falls")
        {
            if (e->type == "city")   add(id, "place_ruin", 0.40f + 0.40f * v);
            if (e->type == "kingdom") add(id, "place_ruin", 0.30f + 0.30f * v);
        }
        else if (category == "wars" || category == "battles" || category == "crusades")
        {
            if (e->type == "city")   add(id, "place_city", 0.65f); // city likely exists if fought over
            if (e->type == "kingdom") add(id, "represent_kingdom", 0.65f);
        }
        else if (category == "crises")
        {
            if (e->type == "city")   add(id, "place_city", 0.60f);
        }

        if (e->type == "artifact")
        {
            // legendary items should have a chance to appear in-game
            add(id, "seed_artifact", 0.30f + 0.20f * v);
        }
        if (e->type == "dungeon")
        {
            add(id, "place_dungeon", 0.55f + 0.25f * v);
        }
    }
}

void WorldHistoryGen::BuildConvenienceLists(WorldHistoryPackage& pkg)
{
    // derive based on spawn intents and entity types
    auto markUniquePush = [](std::vector<uint32_t>& v, uint32_t id)
        {
            if (std::find(v.begin(), v.end(), id) == v.end())
                v.push_back(id);
        };

    for (const auto& ev : pkg.events)
    {
        for (const auto& si : ev.spawnIntents)
        {
            if (si.spawnType == "place_city") markUniquePush(pkg.citiesToPlace, si.entityId);
            else if (si.spawnType == "place_ruin") markUniquePush(pkg.ruinsToPlace, si.entityId);
            else if (si.spawnType == "represent_kingdom") markUniquePush(pkg.kingdomsToRepresent, si.entityId);
            else if (si.spawnType == "seed_artifact") markUniquePush(pkg.artifactsToSeed, si.entityId);
        }
    }
}

WorldHistoryPackage WorldHistoryGen::Generate(const WorldGenSettings& settings)
{
    WorldHistoryPackage pkg;
    pkg.seed = m_seed;
    pkg.settings = settings;

    m_entityIndex.clear();
    m_pkg = &pkg;

    uint32_t rng = (m_seed ? m_seed : 1u);

    const float v = Volatility01(settings);

    // event count influenced by history length (0..4)
    const int base = 8;
    const int eventCount = base + settings.historyLength * 6; // 8..32

    // available categories: everything loaded in templates
    std::vector<std::string> categories;
    categories.reserve(m_data.templates.size());
    for (const auto& kv : m_data.templates)
        if (!kv.second.empty())
            categories.push_back(kv.first);

    if (categories.empty())
        return pkg;

    int year = 1;

    for (int i = 0; i < eventCount; ++i)
    {
        // time steps influenced by history length (longer history spreads years)
        year += RandRange(rng, 1, 6) + settings.historyLength;

        // choose a category (biased by volatility by selecting from a weighted expanded pick)
        // simple approach: pick a random category, but re-roll favoring war-like ones as volatility rises
        std::string cat = PickCategory(rng, settings);
        const EventTemplate* templ = PickTemplateFromCategory(rng, cat);
        if (!templ) continue;
        {
            // 2nd chance roll to increase odds of violent categories
            std::string reroll = categories[RandRange(rng, 0, (int)categories.size() - 1)];
            if (reroll == "wars" || reroll == "battles" || reroll == "crusades" || reroll == "falls" || reroll == "crises")
                cat = reroll;
        }

        HistoricalEvent ev;
        ev.year = year;
        ev.category = cat;

        std::vector<uint32_t> involved;
        ev.title = ResolvePattern(rng, cat, templ->pattern, involved);

        // de-dup involved ids
        std::sort(involved.begin(), involved.end());
        involved.erase(std::unique(involved.begin(), involved.end()), involved.end());
        ev.involvedEntityIds = involved;

        AddSpawnIntents(settings, cat, ev.involvedEntityIds, ev.spawnIntents);

        pkg.events.push_back(std::move(ev));
    }

    // mark entity flags based on spawn intents
    for (auto& ev : pkg.events)
    {
        for (auto& si : ev.spawnIntents)
        {
            for (auto& ent : pkg.entities)
            {
                if (ent.id != si.entityId) continue;

                if (si.spawnType == "place_city" || si.spawnType == "place_dungeon")
                    ent.spawnCandidate = true;
                if (si.spawnType == "place_ruin")
                    ent.ruinCandidate = true;
                if (si.spawnType == "seed_artifact")
                    ent.itemSpawnable = true;
            }
        }
    }

    BuildConvenienceLists(pkg);

    m_pkg = nullptr;
    return pkg;
}
