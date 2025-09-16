#pragma once

#include <string>
#include <vector>
#include <set>

struct Command {
    std::string name;
    std::vector<std::string> args;
    bool isValid = false;

    Command(const std::string& n, const std::vector<std::string>& a, bool valid = true)
        : name(n), args(a), isValid(valid) {}
};

class CommandParser {
public:
    static std::vector<std::string> tokenize(const std::string& input);

    static Command parse(const std::vector<std::string>& tokens);

    static std::string normalizePath(const std::string& path);

private:
    static const std::set<std::string> validCommands;
};

