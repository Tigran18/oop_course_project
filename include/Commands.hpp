#pragma once
#include "ICommand.hpp"
#include "SlideShow.hpp"
#include "Functions.hpp"
#include <iostream>
#include <vector>
#include <map>
#include <string>

class ExitCommand : public ICommand {
public:
    ExitCommand();
    
    void execute() override;
};

class HelpCommand : public ICommand {
public:
    void execute() override;
};

class NextCommand : public ICommand {
    SlideShow& ss;
public:
    NextCommand(SlideShow& s);

    void execute() override;
};

class PrevCommand : public ICommand {
    SlideShow& ss;
public:
    PrevCommand(SlideShow& s);

    void execute() override;
};

class ShowCommand : public ICommand {
    SlideShow& ss;
public:
    ShowCommand(SlideShow& s);
    
    void execute() override;
};

class NextFileCommand : public ICommand {
    size_t& currentIndex;
    std::vector<SlideShow>& slideshows;
public:
    NextFileCommand(size_t& idx, std::vector<SlideShow>& ss);

    void execute() override;
};

class PrevFileCommand : public ICommand {
    size_t& currentIndex;
    std::vector<SlideShow>& slideshows;
public:
    PrevFileCommand(size_t& idx, std::vector<SlideShow>& ss);

    void execute() override;
};

class GotoCommand : public ICommand {
    size_t& currentIndex;
    std::vector<SlideShow>& slideshows;
    std::map<std::string, size_t>& presentationIndex;
    std::vector<std::string>& presentationOrder;
    std::vector<std::string> args;
public:
    GotoCommand(size_t& idx,
                std::vector<SlideShow>& ss,
                std::map<std::string, size_t>& pIndex,
                std::vector<std::string>& pOrder,
                const std::vector<std::string>& arguments);

    void execute() override;
};

class EmptyCommand : public ICommand {
public:
    void execute() override;
};
