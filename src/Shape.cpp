#include "../include/Shape.hpp"
#include <iostream>

Shape::Shape(const std::string& n, int xx, int yy)
    : name(n), x(xx), y(yy) {}

void Shape::draw() const {
    std::cout << "Drawing shape: " << name
              << " at (" << x << ", " << y << ")" << std::endl;
}

const std::string& Shape::getName() const { 
    return name; 
}

int Shape::getX() const { 
    return x; 
}

int Shape::getY() const { 
    return y; 
}
    