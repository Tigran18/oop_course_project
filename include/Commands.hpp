#pragma once
#include "ICommand.hpp"
#include "SlideShow.hpp"
#include "Functions.hpp"
#include <iostream>
#include <vector>
#include <map>
#include <string>

// Exit command needs to modify this flag
class ExitCommand : public ICommand {
    bool& exitFlag;
public:
    ExitCommand(bool& flag) : exitFlag(flag) {

    }
    void execute() override { 
        exitFlag = true; 
        std::cout << "[INFO] Exiting slideshow\n"; 
    }
};

// Help command
class HelpCommand : public ICommand {
public:
    void execute() override {
        std::cout << "Available commands:\n"
              << "  next                  - Go to the next slide in the current presentation\n"
              << "  prev                  - Go to the previous slide in the current presentation\n"
              << "  show                  - Display the current slide\n"
              << "  nextfile              - Switch to the next presentation\n"
              << "  prevfile              - Switch to the previous presentation\n"
              << "  goto <filename> <n>   - Jump to slide number n in the specified presentation file\n"
              << "  goto <n>              - Jump to slide number n in the current presentation\n"
              << "  help                  - Show this help message\n"
              << "  exit                  - Exit the slideshow\n";
    }
};

// Slide navigation commands
class NextCommand : public ICommand {
    SlideShow& ss;
public:
    NextCommand(SlideShow& s) : ss(s) {}
    void execute() override { ss.next(); }
};

class PrevCommand : public ICommand {
    SlideShow& ss;
public:
    PrevCommand(SlideShow& s) : ss(s) {}
    void execute() override { ss.prev(); }
};

class ShowCommand : public ICommand {
    SlideShow& ss;
public:
    ShowCommand(SlideShow& s) : ss(s) {}
    void execute() override { ss.show(); }
};

// Commands that require multi-slideshow context
class NextFileCommand : public ICommand {
    size_t& currentIndex;
    std::vector<SlideShow>& slideshows;
public:
    NextFileCommand(size_t& idx, std::vector<SlideShow>& ss) : currentIndex(idx), slideshows(ss) {}
    void execute() override {
        if (currentIndex + 1 < slideshows.size()) {
            ++currentIndex;
            slideshows[currentIndex].show();
        } else {
            std::cout << "[WARN] Already at last presentation\n";
        }
    }
};

class PrevFileCommand : public ICommand {
    size_t& currentIndex;
    std::vector<SlideShow>& slideshows;
public:
    PrevFileCommand(size_t& idx, std::vector<SlideShow>& ss) : currentIndex(idx), slideshows(ss) {}
    void execute() override {
        if (currentIndex > 0) {
            --currentIndex;
            slideshows[currentIndex].show();
        } else {
            std::cout << "[WARN] Already at first presentation\n";
        }
    }
};

// Goto command
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
                const std::vector<std::string>& arguments)
        : currentIndex(idx), slideshows(ss), presentationIndex(pIndex),
          presentationOrder(pOrder), args(arguments) {}

    void execute() override {
        if (args.empty()) {
            std::cout << "[ERR] Goto requires at least a slide number\n";
            return;
        }
        if (args.size() == 1) {
            try {
                size_t slideNum = std::stoul(args[0]);
                slideshows[currentIndex].gotoSlide(slideNum);
            } catch (...) {
                std::cout << "[ERR] Invalid slide number: " << args[0] << "\n";
            }
        } else if (args.size() == 2) {
            std::string normalizedPath = utils::normalizePath(args[0]);
            try {
                size_t slideNum = std::stoul(args[1]);
                auto it = presentationIndex.find(normalizedPath);
                if (it != presentationIndex.end()) {
                    currentIndex = it->second;
                    slideshows[currentIndex].gotoSlide(slideNum);
                } else {
                    std::cout << "[ERR] Presentation not found: " << args[0] << "\n";
                    std::cout << "Available presentations: ";
                    for (auto& name : presentationOrder) std::cout << name << " ";
                    std::cout << "\n";
                }
            } catch (...) {
                std::cout << "[ERR] Invalid slide number: " << args[1] << "\n";
            }
        } else {
            std::cout << "[ERR] Goto requires one or two arguments: [filename] <slide_number>\n";
        }
    }
};

class EmptyCommand : public ICommand {
public:
    void execute() override { /* do nothing */ }
};
