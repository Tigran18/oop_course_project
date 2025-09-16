#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include "../include/SlideShow.hpp"
#include "../include/CommandParser.hpp"

void displayHelp() {
    std::cout << "Available commands:\n"
    << "  next                  - Go to the next slide in the current presentation\n"
    << "  prev                  - Go to the previous slide in the current presentation\n"
    << "  show                  - Display the current slide\n"
    << "  nextfile              - Switch to the next presentation\n"
    << "  prevfile              - Switch to the previous presentation\n"
    << "  goto <filename> <n>   - Jump to slide number n in the specified presentation file (e.g., pp1.txt or ./pp1.txt)\n"
    << "  goto <n>              - Jump to slide number n in the current presentation\n"
    << "  help                  - Show this help message\n"
    << "  exit                  - Exit the slideshow\n";
}

int main(int argc, char* argv[]) {
    std::map<std::string, SlideShow> presentations;
    std::vector<std::string> presentationOrder;
    std::string currentPresentation;
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            std::string filename = CommandParser::normalizePath(argv[i]);
            SlideShow ss(argv[i]);
            ss.open();
            if (!ss.isEmpty()) {
                presentations.emplace(filename, ss);
                presentationOrder.push_back(filename);
            }
        }
        if (!presentations.empty()) {
            currentPresentation = presentationOrder[0];
        }
    }
    bool exitProgram = false;
    std::cout << "=== Multi-PPTX CLI Slideshow ===\n";
    std::cout << "Type 'help' for a list of commands\n";
    while (!exitProgram) {
        if (!presentations.empty()) {
            try {
                const SlideShow& ss = presentations.at(currentPresentation);
                std::cout << "[" << ss.getFilename()
                          << ", Slide " << (ss.getCurrentSlideIndex() + 1)
                          << "/" << ss.getSlideCount() << "] > ";
            } 
            catch (const std::out_of_range&) {
                std::cout << "[Error: Invalid presentation state] > ";
                currentPresentation = presentationOrder.empty() ? "" : presentationOrder[0];
            }
        } 
        else {
            std::cout << "[No presentations] > ";
        }
        std::string inputLine;
        std::getline(std::cin, inputLine);
        if (std::cin.eof()) {
            std::cout << "[INFO] EOF detected, exiting slideshow\n";
            break;
        }
        Command cmd = CommandParser::parse(CommandParser::tokenize(inputLine));
        if (!cmd.isValid && !cmd.name.empty()) {
            std::cout << "[ERR] Unknown command: " << cmd.name << "\n";
            continue;
        }
        if (presentations.empty() && cmd.name != "exit" && cmd.name != "help") {
            std::cout << "[ERR] No presentations loaded\n";
            continue;
        }
        try {
            SlideShow& ss = presentations.at(currentPresentation);
            if (cmd.name == "next") {
                ss.next();
            } 
            else if (cmd.name == "prev") {
                ss.prev();
            } 
            else if (cmd.name == "show") {
                ss.show();
            } 
            else if (cmd.name == "nextfile") {
                auto it = std::find(presentationOrder.begin(), presentationOrder.end(), currentPresentation);
                if (it != presentationOrder.end() && std::next(it) != presentationOrder.end()) {
                    currentPresentation = *std::next(it);
                    presentations.at(currentPresentation).show();
                } 
                else {
                    std::cout << "[WARN] Already at last presentation\n";
                }
            } 
            else if (cmd.name == "prevfile") {
                auto it = std::find(presentationOrder.begin(), presentationOrder.end(), currentPresentation);
                if (it != presentationOrder.begin()) {
                    currentPresentation = *std::prev(it);
                    presentations.at(currentPresentation).show();
                } 
                else {
                    std::cout << "[WARN] Already at first presentation\n";
                }
            } 
            else if (cmd.name == "goto") {
                if (cmd.args.empty()) {
                    std::cout << "[ERR] Goto requires at least a slide number\n";
                    continue;
                }
                if (cmd.args.size() == 1) {
                    try {
                        size_t slideNum = std::stoul(cmd.args[0]);
                        ss.gotoSlide(slideNum);
                    } 
                    catch (const std::exception& e) {
                        std::cout << "[ERR] Invalid slide number: " << cmd.args[0] << "\n";
                    }
                } 
                else if (cmd.args.size() == 2) {
                    std::string filename = CommandParser::normalizePath(cmd.args[0]);
                    try {
                        size_t slideNum = std::stoul(cmd.args[1]);
                        auto it = presentations.find(filename);
                        if (it != presentations.end()) {
                            currentPresentation = filename;
                            presentations.at(currentPresentation).gotoSlide(slideNum);
                        } 
                        else {
                            std::cout << "[ERR] Presentation not found: " << cmd.args[0] << "\n";
                            std::cout << "Available presentations: ";
                            for (const auto& order : presentationOrder) {
                                std::cout << order << " ";
                            }
                            std::cout << "\n";
                        }
                    } 
                    catch (const std::exception& e) {
                        std::cout << "[ERR] Invalid slide number: " << cmd.args[1] << "\n";
                    }
                } 
                else {
                    std::cout << "[ERR] Goto requires one or two arguments: [filename] <slide_number>\n";
                }
            } 
            else if (cmd.name == "help") {
                displayHelp();
            } 
            else if (cmd.name == "exit") {
                exitProgram = true;
                std::cout << "[INFO] Exiting slideshow\n";
            }
        } 
        catch (const std::out_of_range&) {
            std::cout << "[ERR] Current presentation is invalid. Resetting to first presentation.\n";
            if (!presentationOrder.empty()) {
                currentPresentation = presentationOrder[0];
                presentations.at(currentPresentation).show();
            } 
            else {
                std::cout << "[ERR] No valid presentations available.\n";
            }
        }
    }
    return 0;
}