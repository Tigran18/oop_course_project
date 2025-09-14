#include "../include/Tokenizer.hpp"

std::vector<std::string> Tokenizer::tokenize(const std::string& input, char delimiter) {
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : input) {
        if (ch == delimiter) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } 
        else {
            current.push_back(ch);
        }
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}
