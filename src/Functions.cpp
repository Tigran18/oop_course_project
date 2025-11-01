#include "../include/Functions.hpp"
#include <algorithm>
#include <cctype>

std::string utils::normalizePath(const std::string& path) {
    std::string n = path;
    std::transform(n.begin(), n.end(), n.begin(), ::tolower);
    std::replace(n.begin(), n.end(), '\\', '/');
    if (n.find("./") == 0) {
        n.erase(0, 2);
    }
    else if (n.find(".\\") == 0) {
        n.erase(0, 2);
    }
    return n;
}

std::string utils::trim(const std::string& s) {
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