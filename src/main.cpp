#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include "../include/Functions.hpp"
#include "../include/SlideShow.hpp"
#include "../include/CommandParser.hpp"

void displayHelp() {
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

int main(int argc, char* argv[]) {
    std::vector<SlideShow> slideshows;
    std::map<std::string, size_t> presentationIndex;
    std::vector<std::string> presentationOrder;
    size_t currentIndex = 0;
    if(argc > 1) {
        for (int i = 1; i < argc; ++i) {
            std::string normalizedPath = utils::normalizePath(argv[i]);
            SlideShow ss(normalizedPath);
            ss.open();
            if (!ss.isEmpty()) {
                slideshows.push_back(ss);
                presentationIndex[normalizedPath] = slideshows.size() - 1;
                presentationOrder.push_back(normalizedPath);
            } 
            else {
                std::cout << "[WARN] File empty or invalid: " << normalizedPath << "\n";
            }
        }
        if (!slideshows.empty()) {
            currentIndex = 0;
        }
    }
    std::cout << "=== Multi-PPTX CLI Slideshow ===\n";
    std::cout << "Type 'help' for a list of commands\n";
    bool exitProgram = false;
    while (!exitProgram) {
        if (!slideshows.empty()) {
            const SlideShow& ss = slideshows[currentIndex];;
        } 
        else {
            std::cout << "[No presentations] > ";
        }
        Command cmd = CommandParser::parse(std::cin);
        if (std::cin.eof()) {
            std::cout << "\n[INFO] EOF detected\n";
            break;
        }
        if (cmd.name == "help") {
            displayHelp();
            continue;
        } 
        else if (cmd.name == "exit") {
            std::cout << "[INFO] Exiting slideshow\n";
            break;
        }
        if (slideshows.empty()) {
            std::cout << "[ERR] No presentations loaded\n";
            continue;
        }
        SlideShow& ss = slideshows[currentIndex];
        try {
            if (cmd.name == "next") {
                slideshows[currentIndex].next();
            } 
            else if (cmd.name == "prev") {
                slideshows[currentIndex].prev();
            } 
            else if (cmd.name == "show") {
                slideshows[currentIndex].show();
            } 
            else if (cmd.name == "nextfile") {
                if (currentIndex + 1 < slideshows.size()) {
                    currentIndex++;
                    slideshows[currentIndex].show();
                }
                else {
                    std::cout << "[WARN] Already at last presentation\n";
                }
            } 
            else if (cmd.name == "prevfile") {
                if (currentIndex > 0) {
                    currentIndex--;
                    slideshows[currentIndex].show();
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
                        slideshows[currentIndex].gotoSlide(slideNum);
                    } 
                    catch (...) {
                        std::cout << "[ERR] Invalid slide number: " << cmd.args[0] << "\n";
                    }
                } 
                else if (cmd.args.size() == 2) {
                    std::string normalizedPath = utils::normalizePath(cmd.args[0]);
                    try {
                        size_t slideNum = std::stoul(cmd.args[1]);
                        auto it = presentationIndex.find(normalizedPath);
                        if (it != presentationIndex.end()) {
                            currentIndex = it->second;
                            slideshows[currentIndex].gotoSlide(slideNum);
                        }
                        else {
                            std::cout << "[ERR] Presentation not found: " << cmd.args[0] << "\n";
                            std::cout << "Available presentations: ";
                            for (const auto& name : presentationOrder) {
                                std::cout << name << " ";
                            }
                            std::cout << "\n";
                        }
                    } 
                    catch (...) {
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
            else {
                std::cout << "[WARN] Invalid command.\n";
            }
        } 
        catch (const std::out_of_range&) {
            std::cout << "[ERR] Invalid presentation state. Resetting to first presentation.\n";
            if (!slideshows.empty()) {
                currentIndex = 0;
            }
        }
    }
    return 0;
}
