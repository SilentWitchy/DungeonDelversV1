#pragma once
#include <string>
#include "history/HistoryData.h"

struct HistoryLoadResult
{
    bool ok = false;
    int warnings = 0;
    std::string error; // non-empty when ok==false
};

// Loads assets/data/history_data.txt (or any path you pass).
HistoryLoadResult LoadHistoryDataTxt(const std::string& path, HistoryData& out);
