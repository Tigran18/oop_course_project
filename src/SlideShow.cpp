#include "SlideShow.hpp"
#include "Color.hpp"
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

Slide& SlideShow::currentSlide() {
    if (slides.empty()) {
        throw std::runtime_error("No slides in presentation");
    }
    if (currentIndex >= slides.size()) {
        currentIndex = 0;
    }
    return slides[currentIndex];
}

const Slide& SlideShow::currentSlide() const {
    if (slides.empty()) {
        throw std::runtime_error("No slides in presentation");
    }
    const size_t idx = (currentIndex < slides.size()) ? currentIndex : 0;
    return slides[idx];
}

size_t SlideShow::getCurrentIndex() const {
    return currentIndex;
}

void SlideShow::setCurrentIndex(size_t idx) {
    currentIndex = idx;
}

void SlideShow::next() {
    if (slides.empty()) return;
    currentIndex = (currentIndex + 1) % slides.size();
}

void SlideShow::prev() {
    if (slides.empty()) return;
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
        } else {
            std::cout << "  - Text '" << s.getName() << "' at (" << s.getX() << "," << s.getY() << ")\n";
        }
    }
}
