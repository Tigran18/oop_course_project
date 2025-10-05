#include "../include/Tokenizer.hpp"

std::vector<std::string> Tokenizer::tokenizeCommandLine(const std::string& input) {
    std::vector<std::string> tokens;
    std::string token;
    bool inQuotes = false;
    char quoteChar = '\0';
    for (char c : input) {
        if (c == '"' || c == '\'') {
            if (inQuotes && c == quoteChar) {
                inQuotes = false;
                if (!token.empty()) { 
                    tokens.push_back(token); 
                    token.clear(); 
                }
            } 
            else if (!inQuotes) {
                inQuotes = true;
                quoteChar = c;
            } 
            else {
                token += c;
            }
        } 
        else if (std::isspace(static_cast<unsigned char>(c)) && !inQuotes) {
            if (!token.empty()) { 
                tokens.push_back(token); 
                token.clear(); 
            }
        } 
        else {
            token += c;
        }
    }
    if (!token.empty()) {
        tokens.push_back(token);
    }
    return tokens;
}

std::vector<std::string> Tokenizer::tokenizeSlideShow(const std::string& input, char delimiter) {
    std::vector<std::string> tokens;
    std::string current;
    bool lastWasDelimiter = false;

    for (char ch : input) {
        if (ch == delimiter) {
            if (!current.empty()) {
                tokens.push_back(utils::trim(current));
                current.clear();
            }
            lastWasDelimiter = true;
        } else {
            current.push_back(ch);
            lastWasDelimiter = false;
        }
    }

    if (!current.empty()) {
        tokens.push_back(utils::trim(current));
    } else if (lastWasDelimiter && !tokens.empty()) {
        tokens.push_back("");
    }

    return tokens;
}