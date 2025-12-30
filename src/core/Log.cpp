#include "core/Log.h"
#include <iostream>

namespace logx
{
    void Info(const std::string& s)  { std::cout  << "[INFO] "  << s << "\n"; }
    void Warn(const std::string& s)  { std::cout  << "[WARN] "  << s << "\n"; }
    void Error(const std::string& s) { std::cerr  << "[ERROR] " << s << "\n"; }
}
