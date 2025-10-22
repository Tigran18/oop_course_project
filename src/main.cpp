#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <memory>

#include "../include/Functions.hpp"
#include "../include/SlideShow.hpp"
#include "../include/CommandParser.hpp"
#include "../include/ICommand.hpp"


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
            std::cout << "[pp " << currentIndex+1 << "] > ";
        } 
        else {
            std::cout << "[No presentations] > ";
        }
        auto cmdObj = CommandParser::parse(std::cin,
            slideshows[currentIndex],
            currentIndex,
            slideshows,
            presentationIndex,
            presentationOrder,
            exitProgram);
        if (std::cin.eof()) break;
        cmdObj->execute();
    }
    return 0;
}
