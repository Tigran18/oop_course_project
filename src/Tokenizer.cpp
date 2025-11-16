#include "../include/Tokenizer.hpp"
#include <cctype>

std::vector<Token> Tokenizer::tokenizeCommandLine(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;
    auto skip = [&]() { 
        while (i < line.size() && std::isspace(line[i])) {
             ++i; 
            }
        };
    while (i < line.size()) {
        skip();
        if (i >= line.size()) {
            break;
        }
        if (line[i] == '"') {
            ++i;
            size_t start = i;
            while (i < line.size() && line[i] != '"') {
                ++i;
            }
            std::string str = line.substr(start, i - start);
            if (i < line.size()) {
                ++i;
            }
            tokens.emplace_back(TokenType::STRING, std::move(str));
        }
        else if (std::isdigit(line[i]) || (line[i] == '-' && i+1 < line.size() && std::isdigit(line[i+1]))) {
            size_t start = i;
            if (line[i] == '-') {
                ++i;
            }
            while (i < line.size() && std::isdigit(line[i])) {
                ++i;
            }
            if (i < line.size() && line[i] == '.') { 
                ++i; 
                while (i < line.size() && std::isdigit(line[i])) {
                    ++i; 
                }
            }
            tokens.emplace_back(TokenType::NUMBER, std::stod(line.substr(start, i - start)));
        }
        else {
            size_t start = i;
            while (i < line.size() && !std::isspace(line[i]) && line[i] != '"') {
                ++i;
            }
            std::string word = line.substr(start, i - start);
            TokenType type = tokens.empty() ? TokenType::COMMAND : TokenType::IDENTIFIER;
            tokens.emplace_back(type, std::move(word));
        }
    }
    return tokens;
}