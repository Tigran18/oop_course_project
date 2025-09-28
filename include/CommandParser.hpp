#pragma once

#include <string>
#include <vector>
#include <set>
#include <istream>
#include "Functions.hpp"

struct Command {
    std::string name;
    std::vector<std::string> args;
    bool isValid = false;
    Command(const std::string& n, const std::vector<std::string>& a, bool valid = true)
        : name(n), args(a), isValid(valid) {}
};

class CommandParser {
public:
    static Command parse(std::istream& in);

private:
    static const std::set<std::string> validCommands;
    static std::vector<std::string> tokenize(const std::string& input);
};
