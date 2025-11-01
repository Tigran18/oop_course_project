#pragma once
#include <memory>

class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void execute() = 0;
    virtual bool shouldExit() const;
};