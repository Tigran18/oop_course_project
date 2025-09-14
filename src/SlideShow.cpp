#include "../include/SlideShow.hpp"
#include "../include/Tokenizer.hpp"
#include <iostream>
#include <fstream>

SlideShow::SlideShow(const std::string& filename) : currentIndex(0), filename(filename) {}

static std::string trim(const std::string& s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(static_cast<unsigned char>(*start))) {
        start++;
    }
    auto end = s.end();
    do { 
        end--; 
    } while (std::distance(start, end) > 0 && std::isspace(static_cast<unsigned char>(*end)));
    return std::string(start, end + 1);
}

void SlideShow::open() {
    std::ifstream file(this->filename);
    if (!file.is_open()) {
        std::cout << "[ERR] Could not open file: " << filename << std::endl;
        return;
    }
    slides.clear();
    Slide currentSlide;
    std::string line;
    while (std::getline(file, line)) {
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
            std::vector<std::string> tokens = Tokenizer::tokenize(line, ',');
            if (tokens.size() != 3) {
                std::cout << "[WARN] Ignoring invalid line: " << line << "\n";
                continue;
            }
            try {
                std::string name = trim(tokens[0]);
                int x = std::stoi(trim(tokens[1]));
                int y = std::stoi(trim(tokens[2]));
                currentSlide.addShape(Shape(name, x, y));
            } 
            catch (std::exception& e) {
                std::cout << "[WARN] Failed to parse line: " << line << "\n";
            }
        }
    }
    if (!currentSlide.isEmpty()) {
        slides.push_back(currentSlide);
    }
    currentIndex = 0;
    std::cout << "[INFO] Loaded " << slides.size() << " slides from " << filename << std::endl;
}

void SlideShow::show() const {
    if (slides.empty()) {
        std::cout << "[ERR] No slides to display\n";
        return;
    }
    std::cout << "Showing slide " << (currentIndex + 1)
              << " of " << slides.size() << " in file " << filename << "\n";
    slides[currentIndex].show();
}

void SlideShow::next() {
    if (currentIndex + 1 < slides.size()) {
        currentIndex++;
        show();
    } 
    else {
        std::cout << "[WARN] Already at the last slide\n";
    }
}

void SlideShow::prev() {
    if (currentIndex > 0) {
        currentIndex--;
        show();
    } 
    else {
        std::cout << "[WARN] Already at the first slide\n";
    }
}
