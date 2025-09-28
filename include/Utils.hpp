#pragma once
#include <string>
#include <cctype>

namespace utils {
    inline std::string trim(const std::string& s) {
        auto start = s.begin();
        while (start != s.end() && std::isspace(static_cast<unsigned char>(*start))) {
            ++start;
        }
        auto end = s.end();
        while (end != start && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
            --end;
        }
        return std::string(start, end);
    }
}