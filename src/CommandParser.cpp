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

std::string CommandParser::normalizePath(const std::string& path) {
    std::string normalized = path;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { 
        return std::tolower(c); 
    });
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    if (normalized.find("./") == 0) {
        normalized.erase(0, 2);
    } 
    else if (normalized.find(".\\") == 0) {
        normalized.erase(0, 2);
    }
    return normalized;
}

std::string trim(const std::string& s) {
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

Command CommandParser::parse(const std::vector<std::string>& tokens) {
    if (tokens.empty()) {
        return Command("", {}, false);
    }
    std::string command = tokens[0];
    std::transform(command.begin(), command.end(), command.begin(),
                   [](unsigned char c) { 
        return std::tolower(c); 
    });
    if (validCommands.find(command) == validCommands.end()) {
        return Command(command, {}, false);
    }
    std::vector<std::string> args(tokens.begin() + 1, tokens.end());
    if (command == "goto") {
        if (args.empty()) {
            return Command(command, args, false); 
        }
        for (auto& arg : args) {
            arg = trim(arg);
        }
        if (args.size() > 2) {
            return Command(command, args, false); 
        }
    } 
    else {
        for (auto& arg : args) {
            arg = trim(arg);
        }
    }
    return Command(command, args, true);
}