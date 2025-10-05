#pragma once

#include "Functions.hpp"
#include <string>
#include <vector>

class Tokenizer {
public:
    static std::vector<std::string> tokenizeCommandLine(const std::string& input);

    static std::vector<std::string> tokenizeSlideShow(const std::string& input, char delimiter);
};