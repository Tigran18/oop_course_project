#pragma once

#include <vector>
#include <string>
#include "Slide.hpp"

class SlideShow {
private:
    std::vector<Slide> slides;
    size_t currentIndex;
    std::string filename;
public:
    SlideShow(const std::string& filename);

    void open();
    void show() const;                       
    void next();                
    void prev();
};