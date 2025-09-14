#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "../include/SlideShow.hpp"
#include "../include/Tokenizer.hpp"

int main(int argc, char* argv[]) {
    std::vector<SlideShow> presentations;
    size_t currentPresentation = 0;
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            SlideShow ss(argv[i]);
            ss.open();
            presentations.push_back(ss);
        }
    }
    bool exitProgram = false;
    std::cout << "=== Multi-PPTX CLI Slideshow ===\n";
    std::cout << "Commands: next, prev, show, nextfile, prevfile, exit\n";
    while (!exitProgram) {
        std::cout << "> ";
        std::string cmd;
        std::cin >> cmd;
        if (presentations.empty()) {
            if (cmd == "exit") {
                exitProgram = true;
                std::cout << "[INFO] Exiting slideshow\n";
            }
            else{
                std::cout << "[ERR] No presentations loaded\n";
            }
            continue;
        }
        SlideShow& ss = presentations[currentPresentation];
        if (cmd == "next") {
            ss.next();
        }
        else if (cmd == "prev") {
            ss.prev();
        }
        else if (cmd == "show") {
            ss.show();
        }
        else if (cmd == "nextfile") {
            if (currentPresentation < presentations.size() - 1) {
                currentPresentation++;
                presentations[currentPresentation].show();
            }
            else {
                std::cout << "[WARN] Already at last presentation\n";
            }
        }
        else if (cmd == "prevfile") {
            if (currentPresentation > 0) {
                currentPresentation--;
                presentations[currentPresentation].show();
            }
            else {
                std::cout << "[WARN] Already at first presentation\n";
            }
        }
        else if (cmd == "exit") {
            exitProgram = true;
            std::cout << "[INFO] Exiting slideshow\n";
        }
        else std::cout << "[ERR] Unknown command: " << cmd << "\n";
    }
    return 0;
}
