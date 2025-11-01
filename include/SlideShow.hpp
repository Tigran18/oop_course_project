#pragma once
#include "Slide.hpp"
#include <string>
#include <vector>

class SlideShow {
private:
    std::vector<Slide> slides;
    size_t currentIndex = 0;
    std::string filename;

public:
    explicit SlideShow(const std::string& filename);

    void open();
    void show() const;
    void next();
    void prev();
    void gotoSlide(size_t slideNumber);

    const std::vector<Slide>& getSlides() const;
    std::vector<Slide>& getSlides();
    size_t getCurrentIndex() const;
    void   setCurrentIndex(size_t idx);
    const std::string& getFilename() const;

    bool isEmpty() const;
};