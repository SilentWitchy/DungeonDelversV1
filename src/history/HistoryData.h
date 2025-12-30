#pragma once
#include <string>
#include <unordered_map>
#include <vector>

struct NameEntry
{
    std::string text;
    std::vector<std::string> tags; // lowercase, trimmed
};

struct EventTemplate
{
    std::string category; // lowercase
    std::string pattern;
};

struct HistoryData
{
    // Names with tags
    std::vector<NameEntry> names;

    // POOL:<name> -> entries
    std::unordered_map<std::string, std::vector<std::string>> pools; // lowercase key

    // TEMPLATE:<category> -> list
    std::unordered_map<std::string, std::vector<EventTemplate>> templates; // lowercase key
};
