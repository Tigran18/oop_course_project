#pragma once
#include "Slide.hpp"
#include <string>
#include <vector>

class SlideShow {
private:
    std::string filename;
    std::vector<Slide> slides;
    size_t currentIndex = 0;

public:
    explicit SlideShow(std::string name);

    const std::string& getFilename() const;
    std::vector<Slide>& getSlides();
    const std::vector<Slide>& getSlides() const;

    // Convenience accessors used by the GUI.
    // Throws if there are no slides.
    Slide& currentSlide();
    const Slide& currentSlide() const;
    size_t getCurrentIndex() const;
    void setCurrentIndex(size_t idx);

    void next();
    void prev();
    void show() const;

    SlideShow(const SlideShow&) = default;
    SlideShow& operator=(const SlideShow&) = default;
};
