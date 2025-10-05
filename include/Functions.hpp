#include <string>
#include <algorithm>
#include <filesystem>
#include <iostream>


namespace utils{
    std::string normalizePath(const std::string& path);

    std::string trim(const std::string& s);

    void listFiles(const std::string& directory, const std::string& extension);
}