#include "../include/SlideShow.hpp"
#include "../include/Tokenizer.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>

SlideShow::SlideShow(const std::string& filename) : currentIndex(0), filename(filename) {}

static std::string trim(const std::string& s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(static_cast<unsigned char>(*start))) {
        ++start;
    }
    auto end = s.end();
    while (end != start && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
        --end;
    }
    return std::string(start, end);
}

void SlideShow::open() {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "[ERR] Could not open file: " << filename << "\n";
        return;
    }
    slides.clear();
    Slide currentSlide;
    std::string line;
    size_t lineNumber = 0;
    while (std::getline(file, line)) {
        ++lineNumber;
        line = trim(line);
        if (line.empty()) {
            continue;
        }
        if (line == "---") {
            if (!currentSlide.isEmpty()) {
                slides.push_back(currentSlide);
                currentSlide = Slide();
            }
        } 
        else {
            std::vector<std::string> tokens = Tokenizer::tokenizeSlideShow(line, ',');
            if (tokens.size() != 3) {
                std::cout << "[WARN] Ignoring invalid line " << lineNumber << ": " << line << "\n";
                continue;
            }
            try {
                std::string name = trim(tokens[0]);
                if (name.empty()) {
                    std::cout << "[WARN] Ignoring line " << lineNumber << ": Empty shape name\n";
                    continue;
                }
                int x = std::stoi(trim(tokens[1]));
                int y = std::stoi(trim(tokens[2]));
                currentSlide.addShape(Shape(name, x, y));
            } 
            catch (const std::exception& e) {
                std::cout << "[WARN] Failed to parse line " << lineNumber << ": " << line << " (" << e.what() << ")\n";
            }
        }
    }
    if (!currentSlide.isEmpty()) {
        slides.push_back(currentSlide);
    }
    currentIndex = 0;
    if (slides.empty()) {
        std::cout << "[INFO] No valid slides loaded from " << filename << "\n";
    }
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
    if (currentIndex + 1 < slides.size()) {
        ++currentIndex;
        show();
    } 
    else {
        std::cout << "[WARN] Already at the last slide of " << filename << "\n";
    }
}

void SlideShow::prev() {
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