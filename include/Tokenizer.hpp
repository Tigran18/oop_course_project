#pragma once
#include <string>
#include <vector>
#include "Token.hpp"

class Tokenizer {
public:
    static std::vector<Token> tokenizeCommandLine(const std::string& line);
};