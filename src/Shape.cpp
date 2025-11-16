#include "../include/Shape.hpp"

Shape::Shape(std::string n, int px, int py)
    : name(std::move(n)), x(px), y(py) {}

Shape::Shape(std::string n, int px, int py, std::vector<uint8_t> data)
    : name(std::move(n)), x(px), y(py), imageData(std::move(data)), isImageFlag(true) {}

const std::string& Shape::getName() const { 
    return name; 
}

int Shape::getX() const { 
    return x; 
}

int Shape::getY() const { 
    return y; 
}

const std::vector<uint8_t>& Shape::getImageData() const { 
    return imageData; 
}

bool Shape::isImage() const { 
    return isImageFlag; 
}