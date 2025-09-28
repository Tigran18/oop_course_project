#include "../include/CommandParser.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

const std::set<std::string> CommandParser::validCommands = {
    "next", "prev", "show", "nextfile", "prevfile", "exit", "help", "goto"
};

std::vector<std::string> CommandParser::tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::string token;
    bool inQuotes = false;
    char quoteChar = '\0';
    for (size_t i = 0; i < input.length(); ++i) {
        char c = input[i];
        if (c == '"' || c == '\'') {
            if (inQuotes && c == quoteChar) {
                inQuotes = false;
                if (!token.empty()) { 
                    tokens.push_back(token); token.clear(); 
                }
            } 
            else if (!inQuotes) {
                inQuotes = true; quoteChar = c;
            } 
            else {
                token += c;
            }
        } 
        else if (std::isspace(static_cast<unsigned char>(c)) && !inQuotes) {
            if (!token.empty()) { 
                tokens.push_back(token); token.clear(); 
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

Command CommandParser::parse(std::istream& in) {
    std::string line;
    std::getline(in, line);
    if (in.eof()) {
        return Command("", {}, false);
    }
    auto tokens = tokenize(line);
    if (tokens.empty()) {
        return Command("", {}, false);
    }
    std::string command = tokens[0];
    std::transform(command.begin(), command.end(), command.begin(),
                   [](unsigned char c){ 
                    return std::tolower(c); 
                });
    if (validCommands.find(command) == validCommands.end()) {
        return Command(command, {}, false);
    }
    std::vector<std::string> args(tokens.begin() + 1, tokens.end());
    for (auto& arg : args) {
        arg = trim(arg);
    }
    if (command == "goto") {
        if (args.empty() || args.size() > 2) {
            return Command(command, args, false);
        }
    }
    return Command(command, args, true);
}
