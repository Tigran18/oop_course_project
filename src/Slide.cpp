#include "../include/Slide.hpp"
#include <iostream>

void Slide::addShape(const Shape& shape) {
    shapes.push_back(shape);
}

void Slide::show() const {
    std::cout << "Slide contains " << shapes.size() << " shapes:\n";
    for (const auto& s : shapes) {
        s.draw();
    }
}

bool Slide::isEmpty() const {
    return shapes.empty();
}

const std::vector<Shape>& Slide::getShapes() const { 
    return shapes; 
}

std::vector<Shape>& Slide::getShapes() { 
    return shapes; 
}