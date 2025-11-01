#include "../include/Tokenizer.hpp"
#include <cctype>

std::vector<Token> Tokenizer::tokenizeCommandLine(const std::string& input) {
    std::vector<Token> tokens;
    std::string current;
    bool inQuotes = false;
    char quoteChar = '\0';
    for (char c : input) {
        if (c == '"' || c == '\'') {
            if (inQuotes && c == quoteChar) {
                current.push_back(c);
                tokens.emplace_back(TokenType::QUOTED_STRING, std::move(current));
                current.clear();
                inQuotes = false;
            } else if (!inQuotes) {
                if (!current.empty()) {
                    tokens.emplace_back(TokenType::LITERAL, std::move(current));
                    current.clear();
                }
                inQuotes = true;
                quoteChar = c;
                current.push_back(c);
            } 
            else {
                current.push_back(c);
            }
        } 
        else if (std::isspace(static_cast<unsigned char>(c)) && !inQuotes) {
            if (!current.empty()) {
                TokenType t = (tokens.empty() ? TokenType::COMMAND : TokenType::LITERAL);
                tokens.emplace_back(t, std::move(current));
                current.clear();
            }
        } 
        else {
            current.push_back(c);
        }
    }
    if (!current.empty()) {
        TokenType t = (tokens.empty() ? TokenType::COMMAND : TokenType::LITERAL);
        tokens.emplace_back(t, std::move(current));
    }
    return tokens;
}