#pragma once
#include "ICommand.hpp"
#include "SlideShow.hpp"
#include <vector>
#include <map>
#include <string>
#include <iostream>

class Controller;

class CommandExit : public ICommand {
public:
    void execute() override;
};

class CommandHelp : public ICommand {
public:
    void execute() override;
};

class CommandEmpty : public ICommand {
public:
    void execute() override;
};

class CommandSafety : public ICommand {
    std::string message;
public:
    CommandSafety(const std::string& msg) : message(msg) {}
    void execute() override { std::cout << message << "\n"; }
};


class CommandNext : public ICommand {
    SlideShow& ss;
public:
    CommandNext(SlideShow& s) : ss(s) {}
    void execute() override { ss.next(); }
};

class CommandPrev : public ICommand {
    SlideShow& ss;
public:
    CommandPrev(SlideShow& s) : ss(s) {}
    void execute() override { ss.prev(); }
};

class CommandShow : public ICommand {
    SlideShow& ss;
public:
    CommandShow(SlideShow& s) : ss(s) {}
    void execute() override { ss.show(); }
};

class CommandNextFile : public ICommand {
    size_t& currentIndex;
    std::vector<SlideShow>& slideshows;
public:
    CommandNextFile(size_t& idx, std::vector<SlideShow>& ss)
        : currentIndex(idx), slideshows(ss) {}
    void execute() override;
};

class CommandPrevFile : public ICommand {
    size_t& currentIndex;
    std::vector<SlideShow>& slideshows;
public:
    CommandPrevFile(size_t& idx, std::vector<SlideShow>& ss)
        : currentIndex(idx), slideshows(ss) {}
    void execute() override;
};

class CommandGoto : public ICommand {
    size_t& currentIndex;
    std::vector<SlideShow>& slideshows;
    std::map<std::string, size_t>& presentationIndex;
    std::vector<std::string>& presentationOrder;
    std::vector<std::string> args;
public:
    CommandGoto(size_t& idx, std::vector<SlideShow>& ss,
                std::map<std::string, size_t>& pIndex,
                std::vector<std::string>& pOrder,
                const std::vector<std::string>& arguments);
    void execute() override;
};

class CommandSave : public ICommand {
    Controller& ctrl;
    std::vector<std::string> args;
public:
    CommandSave(Controller& c, const std::vector<std::string>& a) : ctrl(c), args(a) {}
    void execute() override;
};

class CommandLoad : public ICommand {
    Controller& ctrl;
    std::vector<std::string> args;
public:
    CommandLoad(Controller& c, const std::vector<std::string>& a) : ctrl(c), args(a) {}
    void execute() override;
};

class CommandAdd : public ICommand {
    Controller& ctrl;
    std::vector<std::string> args;
public:
    CommandAdd(Controller& c, const std::vector<std::string>& a) : ctrl(c), args(a) {}
    void execute() override;
};