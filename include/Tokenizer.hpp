#pragma once
#include "Token.hpp"
#include <vector>

class Tokenizer {
public:
    static std::vector<Token> tokenizeCommandLine(const std::string& input);
};