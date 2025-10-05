#include "../include/Functions.hpp"

std::string utils::normalizePath(const std::string& path) {
    std::string normalized = path;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { 
        return std::tolower(c); 
    });
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    if (normalized.find("./") == 0) {
        normalized.erase(0, 2);
    } 
    else if (normalized.find(".\\") == 0) {
        normalized.erase(0, 2);
    }
    return normalized;
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