#include "history/HistoryDataLoader.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

static std::string Trim(std::string s)
{
    auto is_ws = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!s.empty() && is_ws((unsigned char)s.front())) s.erase(s.begin());
    while (!s.empty() && is_ws((unsigned char)s.back()))  s.pop_back();
    return s;
}

static std::string ToLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

static bool StartsWith(const std::string& s, const std::string& prefix)
{
    return s.size() >= prefix.size() && std::equal(prefix.begin(), prefix.end(), s.begin());
}

static bool IsCommentOrEmpty(const std::string& raw)
{
    std::string s = Trim(raw);
    if (s.empty()) return true;
    if (StartsWith(s, "#")) return true;
    if (StartsWith(s, "//")) return true;
    return false;
}

static std::vector<std::string> Split(const std::string& s, char delim)
{
    std::vector<std::string> out;
    std::string cur;
    std::istringstream iss(s);
    while (std::getline(iss, cur, delim))
        out.push_back(cur);
    return out;
}

static bool ParseSectionHeader(const std::string& raw, std::string& outHeaderInner)
{
    std::string s = Trim(raw);
    if (s.size() < 3) return false;
    if (s.front() != '[' || s.back() != ']') return false;
    outHeaderInner = Trim(s.substr(1, s.size() - 2));
    return !outHeaderInner.empty();
}

static bool ParseNameLine(const std::string& raw, NameEntry& out)
{
    // <text> | tags:<tag1>,<tag2>
    auto parts = Split(raw, '|');
    if (parts.size() < 2) return false;

    std::string left = Trim(parts[0]);
    std::string right = Trim(parts[1]);

    if (left.empty()) return false;

    std::string rightLower = ToLower(right);
    if (!StartsWith(rightLower, "tags:")) return false;

    std::string tagsPart = Trim(right.substr(5)); // after "tags:"
    if (tagsPart.empty()) return false;

    auto tags = Split(tagsPart, ',');
    std::vector<std::string> cleaned;
    cleaned.reserve(tags.size());
    for (auto& t : tags)
    {
        std::string tag = ToLower(Trim(t));
        if (!tag.empty())
            cleaned.push_back(tag);
    }
    if (cleaned.empty()) return false;

    out.text = left;
    out.tags = std::move(cleaned);
    return true;
}

// src/history/HistoryDataLoader.cpp

static bool ParseTemplateLine(const std::string& raw, std::string& outPattern)
{
    // pattern-only line
    std::string pattern = Trim(raw);
    if (pattern.empty()) return false;
    outPattern = std::move(pattern);
    return true;
}

HistoryLoadResult LoadHistoryDataTxt(const std::string& path, HistoryData& out)
{
    HistoryLoadResult res;

    std::ifstream in(path);
    if (!in.is_open())
    {
        res.ok = false;
        res.error = "Failed to open: " + path;
        return res;
    }

    out = HistoryData{}; // reset

    enum class SecType { None, Name, Pool, Template, Settings };
    SecType secType = SecType::None;
    std::string secKey; // for POOL/TEMPLATE name, lowercase

    std::string line;
    int lineNo = 0;

    while (std::getline(in, line))
    {
        ++lineNo;
        if (IsCommentOrEmpty(line))
            continue;

        std::string hdrInner;
        if (ParseSectionHeader(line, hdrInner))
        {
            std::string h = ToLower(Trim(hdrInner));
            secKey.clear();

            if (h == "name")
            {
                secType = SecType::Name;
                continue;
            }
            if (h == "settings")
            {
                secType = SecType::Settings;
                continue;
            }

            // POOL:<x> or TEMPLATE:<x>
            auto colonPos = h.find(':');
            if (colonPos != std::string::npos)
            {
                std::string head = Trim(h.substr(0, colonPos));
                std::string tail = Trim(h.substr(colonPos + 1));

                if (head == "pool" && !tail.empty())
                {
                    secType = SecType::Pool;
                    secKey = tail; // already lowercase
                    // ensure key exists
                    (void)out.pools[secKey];
                    continue;
                }
                if (head == "template" && !tail.empty())
                {
                    secType = SecType::Template;
                    secKey = tail; // already lowercase
                    (void)out.templates[secKey];
                    continue;
                }
            }

            // unknown header
            res.warnings++;
            secType = SecType::None;
            continue;
        }

        // content lines
        if (secType == SecType::None || secType == SecType::Settings)
        {
            // ignore unknown/unhandled content
            res.warnings++;
            continue;
        }

        if (secType == SecType::Name)
        {
            NameEntry e;
            if (!ParseNameLine(line, e))
            {
                res.warnings++;
                continue;
            }
            out.names.push_back(std::move(e));
            continue;
        }

        if (secType == SecType::Pool)
        {
            std::string entry = Trim(line);
            if (entry.empty())
            {
                res.warnings++;
                continue;
            }
            out.pools[secKey].push_back(std::move(entry));
            continue;
        }

        if (secType == SecType::Template)
        {
            std::string pat;
            if (!ParseTemplateLine(line, pat))
            {
                res.warnings++;
                continue;
            }

            EventTemplate t;
            t.category = secKey;
            t.pattern = std::move(pat);
            out.templates[secKey].push_back(std::move(t));
            continue;
        }

        // If we got here, it's content in an unexpected section
        res.warnings++;
        continue;
    }

    // minimal validity: need at least one template somewhere
    bool hasAnyTemplate = false;
    for (auto& kv : out.templates)
    {
        if (!kv.second.empty()) { hasAnyTemplate = true; break; }
    }
    if (!hasAnyTemplate)
    {
        res.ok = false;
        res.error = "No templates loaded from: " + path;
        return res;
    }

    res.ok = true;
    return res;
}
