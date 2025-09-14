#include "../include/Shape.hpp"

Shape::Shape(const std::string& name, int x, int y)
    : name(name), x(x), y(y) {}

void Shape::draw() const {
    std::cout << "Drawing shape: " << name
              << " at (" << x << ", " << y << ")" << std::endl;
}
