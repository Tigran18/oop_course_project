#include "../include/SlideShow.hpp"
#include "../include/Color.hpp"
#include <iostream>

SlideShow::SlideShow(std::string name) : filename(std::move(name)) {}

const std::string& SlideShow::getFilename() const { 
    return filename; 
}

std::vector<Slide>& SlideShow::getSlides() { 
    return slides; 
}

const std::vector<Slide>& SlideShow::getSlides() const { 
    return slides; 
}

size_t SlideShow::getCurrentIndex() const { 
    return currentIndex; 
}

void SlideShow::setCurrentIndex(size_t idx) { 
    currentIndex = idx; 
}

void SlideShow::next() {
    if (slides.empty()) {
        return;
    }
    currentIndex = (currentIndex + 1) % slides.size();
}

void SlideShow::prev() {
    if (slides.empty()) {
        return;
    }
    currentIndex = (currentIndex == 0) ? slides.size() - 1 : currentIndex - 1;
}

void SlideShow::show() const {
    if (slides.empty()) {
        info() << "No slides\n";
        return;
    }
    info() << "Slide " << (currentIndex + 1) << "/" << slides.size() << "\n";
    const auto& shapes = slides[currentIndex].getShapes();
    for (const auto& s : shapes) {
        if (s.isImage()) {
            std::cout << "  - Image '" << s.getName() << "' at (" << s.getX() << "," << s.getY() << ")\n";
        }
        else {
            std::cout << "  - Text '" << s.getName() << "' at (" << s.getX() << "," << s.getY() << ")\n";
        }
    }
}