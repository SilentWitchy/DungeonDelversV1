#pragma once
#include <string>

namespace logx
{
    void Info(const std::string& s);
    void Warn(const std::string& s);
    void Error(const std::string& s);
}
