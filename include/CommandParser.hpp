#pragma once
#include <istream>
#include <memory>

class ICommand;

class CommandParser {
public:
    static std::unique_ptr<ICommand> parse(std::istream& in);
};
