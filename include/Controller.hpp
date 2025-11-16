#pragma once
#include <vector>
#include <deque>
#include <string>
#include <map>

#include "SlideShow.hpp"

class Controller {
private:
    std::vector<SlideShow> slideshows;
    std::map<std::string, size_t> presentationIndex;
    std::vector<std::string> presentationOrder;
    size_t currentIndex = 0;
    bool autoSaveOnExit = true;

    std::deque<std::vector<SlideShow>> undoStack;
    std::deque<std::vector<SlideShow>> redoStack;

    Controller() = default;

    void pushSnapshot();
    void rebuildIndexAndOrder();

public:
    static Controller& instance();

    std::vector<SlideShow>& getSlideshows();
    
    const std::vector<SlideShow>& getSlideshows() const;

    std::map<std::string, size_t>& getPresentationIndex();

    std::vector<std::string>& getPresentationOrder();

    size_t& getCurrentIndex();

    SlideShow& getCurrentSlideshow();
    
    bool getAutoSaveOnExit() const;

    void setAutoSaveOnExit(bool b);

    bool undo();
    bool redo();

    void run();
};
