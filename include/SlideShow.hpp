#pragma once

#include <vector>
#include <string>
#include "Slide.hpp"

class SlideShow {
public:
    explicit SlideShow(const std::string& filename);
    void open();
    void show() const;
    void next();
    void prev();
    void gotoSlide(size_t slideNumber);
    bool isEmpty() const 
    { 
        return slides.empty(); 
    }
    size_t getSlideCount() const 
    { 
        return slides.size(); 
    }
    size_t getCurrentSlideIndex() const 
    { 
        return currentIndex; 
    }
    const std::string& getFilename() const 
    { 
        return filename; 
    }

private:
    std::vector<Slide> slides;
    size_t currentIndex;
    std::string filename;
};