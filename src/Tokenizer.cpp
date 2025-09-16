#include "../include/Tokenizer.hpp"
#include <algorithm>

static std::string trim(const std::string& s) {
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

std::vector<std::string> Tokenizer::tokenize(const std::string& input, char delimiter) {
    std::vector<std::string> tokens;
    std::string current;
    bool lastWasDelimiter = false;

    for (char ch : input) {
        if (ch == delimiter) {
            if (!current.empty()) {
                tokens.push_back(trim(current));
                current.clear();
            }
            lastWasDelimiter = true;
        } else {
            current.push_back(ch);
            lastWasDelimiter = false;
        }
    }

    if (!current.empty()) {
        tokens.push_back(trim(current));
    } else if (lastWasDelimiter && !tokens.empty()) {
        tokens.push_back("");
    }

    return tokens;
}