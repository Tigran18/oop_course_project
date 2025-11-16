#pragma once
#include <memory>
#include <istream>
#include "../include/ICommand.hpp"

class CommandParser {
public:
    std::unique_ptr<ICommand> parse(std::istream& in);
};