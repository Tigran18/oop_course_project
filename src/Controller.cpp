#include "../include/Controller.hpp"
#include "../include/Functions.hpp"
#include "../include/Serializer.hpp"
#include "../include/CommandParser.hpp"
#include <iostream>

Controller::Controller(int argc, char* argv[])
    : storedArgc(argc), storedArgv(argv) {}

Controller& Controller::instance(int argc, char** argv) {
    static Controller theOne(argc, argv);
    return theOne;
}

void Controller::run() {
    static bool filesLoaded = false;
    if (!filesLoaded && storedArgc > 1) {
        for (int i = 1; i < storedArgc; ++i) {
            std::string original = storedArgv[i];
            std::string path     = utils::normalizePath(original);
            std::string binPath  = path + ".bin";
            std::vector<SlideShow> tmp;
            std::map<std::string, size_t> idx;
            std::vector<std::string> order;
            size_t dummy = 0;
            if (Serializer::load(tmp, idx, order, dummy, binPath)) {
                    for (size_t j = 0; j < tmp.size(); ++j) {
                    std::string name = order[j];
                    slideshows.push_back(std::move(tmp[j]));
                    presentationIndex[name] = slideshows.size() - 1;
                    presentationOrder.push_back(name);
                }
            std::cout << "[INFO] Loaded session with " << tmp.size()
                          << " presentation(s) from " << binPath << "\n";
            auto it = idx.find(path);
                if (it != idx.end()) {
                    currentIndex = it->second;
                }
                continue;
            }
            SlideShow ss(path);
            ss.open();
            slideshows.push_back(std::move(ss));
            presentationIndex[path] = slideshows.size() - 1;
            presentationOrder.push_back(path);
        }
        if (!slideshows.empty() && currentIndex >= slideshows.size())
            currentIndex = 0;

        filesLoaded = true;
    }
    std::cout << "=== Multi-PPTX CLI Slideshow ===\n";
    std::cout << "Type 'help' for commands\n";
    while (true) {
        if (!slideshows.empty())
            std::cout << "[pp " << currentIndex + 1 << "/" << slideshows.size() << "] > ";
        else
            std::cout << "[No presentations] > ";

        auto cmd = CommandParser::parse(std::cin);
        if (std::cin.eof()) {
            break;
        }
        cmd->execute();
    }
}

std::vector<SlideShow>& Controller::getSlideshows() { 
    return slideshows; 
}

const std::vector<SlideShow>& Controller::getSlideshows() const { 
    return slideshows; 
}

size_t& Controller::getCurrentIndex() { 
    return currentIndex; 
}

std::map<std::string, size_t>& Controller::getPresentationIndex() { 
    return presentationIndex; 
}

const std::map<std::string, size_t>& Controller::getPresentationIndex() const { 
    return presentationIndex; 
}

std::vector<std::string>& Controller::getPresentationOrder() { 
    return presentationOrder; 
}

SlideShow& Controller::getCurrentSlideshow() {
    return slideshows[currentIndex];
}