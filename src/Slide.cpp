#include "../include/Slide.hpp"
#include <iostream>

void Slide::addShape(const Shape& shape) {
    shapes.push_back(shape);
}

void Slide::show() const {
    std::cout << "Slide contains " << shapes.size() << " shapes:\n";
    for (size_t i = 0; i < shapes.size(); ++i) {
        shapes[i].draw();
    }
}

bool Slide::isEmpty() const {
    return shapes.empty();
}

