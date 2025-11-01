#pragma once
#include "ICommand.hpp"
#include "Token.hpp"
#include <memory>
#include <istream>

class CommandParser {
public:
    static std::unique_ptr<ICommand> parse(std::istream& in);
};