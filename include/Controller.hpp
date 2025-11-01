#pragma once
#include "SlideShow.hpp"
#include <vector>
#include <map>

class Controller {
private:
    std::vector<SlideShow> slideshows;
    std::map<std::string, size_t> presentationIndex;
    std::vector<std::string> presentationOrder;
    size_t currentIndex = 0;

    int storedArgc = 0;
    char** storedArgv = nullptr;

    Controller(int argc, char* argv[]);

public:
    static Controller& instance(int argc = 0, char** argv = nullptr);

    void run();

    std::vector<SlideShow>& getSlideshows();

    const std::vector<SlideShow>& getSlideshows() const;

    size_t& getCurrentIndex();

    std::map<std::string, size_t>& getPresentationIndex();

    const std::map<std::string, size_t>& getPresentationIndex() const;

    std::vector<std::string>& getPresentationOrder();

    SlideShow& getCurrentSlideshow();
};