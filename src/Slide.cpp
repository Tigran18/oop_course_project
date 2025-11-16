#include "../include/Slide.hpp"

void Slide::addShape(Shape s) { 
    shapes.push_back(std::move(s)); 
}

std::vector<Shape>& Slide::getShapes() { 
    return shapes; 
}

const std::vector<Shape>& Slide::getShapes() const { 
    return shapes; 
}

bool Slide::isEmpty() const { 
    return shapes.empty(); 
}