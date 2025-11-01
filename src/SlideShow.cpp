#include "../include/SlideShow.hpp"
#include "../include/Serializer.hpp"
#include <iostream>

SlideShow::SlideShow(const std::string& fn) : filename(fn) {}

void SlideShow::open() {
    slides.clear();
    currentIndex = 0;
    std::string sessionPath = filename + ".bin";
    std::vector<SlideShow> tmp;
    std::map<std::string, size_t> idx;
    std::vector<std::string> ord;
    size_t dummy = 0;
    if (Serializer::load(tmp, idx, ord, dummy, sessionPath)) {
        auto it = idx.find(filename);
        if (it != idx.end() && it->second < tmp.size()) {
            *this = std::move(tmp[it->second]);
            return;
        }
    }
    std::cout << "[INFO] Created new in-memory presentation: " << filename << "\n";
}

void SlideShow::show() const {
    if (slides.empty()) {
        std::cout << "[ERR] No slides to display in " << filename << "\n";
        return;
    }
    std::cout << "Showing slide " << (currentIndex + 1) << " of " << slides.size()
              << " in " << filename << "\n";
    slides[currentIndex].show();
}

void SlideShow::next() {
    if (slides.empty()) { 
        std::cout << "[ERR] No slides in this presentation.\n"; 
        return; 
    }
    if (currentIndex + 1 < slides.size()) { 
        ++currentIndex; 
        show(); 
    }
    else {
        std::cout << "[WARN] Already at the last slide of " << filename << "\n";
    }
}

void SlideShow::prev() {
    if (slides.empty()) { 
        std::cout << "[ERR] No slides in this presentation.\n"; 
        return; 
    }
    if (currentIndex > 0) { 
        --currentIndex; 
        show(); 
    }
    else {
        std::cout << "[WARN] Already at the first slide of " << filename << "\n";
    }
}

void SlideShow::gotoSlide(size_t slideNumber) {
    if (slideNumber == 0 || slideNumber > slides.size()) {
        std::cout << "[ERR] Invalid slide number: " << slideNumber
                  << " (must be between 1 and " << slides.size() << ")\n";
        return;
    }
    currentIndex = slideNumber - 1;
    show();
}

const std::vector<Slide>& SlideShow::getSlides() const { 
    return slides; 
}

std::vector<Slide>& SlideShow::getSlides() { 
    return slides; 
}

size_t SlideShow::getCurrentIndex() const { 
    return currentIndex; 
}

void SlideShow::setCurrentIndex(size_t idx) { 
    currentIndex = idx; 
}

const std::string& SlideShow::getFilename() const { 
    return filename; 
}

bool SlideShow::isEmpty() const { 
    return slides.empty(); 
}