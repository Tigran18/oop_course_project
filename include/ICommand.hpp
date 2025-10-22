#pragma once
#include <vector>
#include <string>

class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void execute() = 0;
};