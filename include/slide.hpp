#pragma once

#include <vector>
#include "Shape.hpp"

class Slide {
private:
    std::vector<Shape> shapes;

public:
    void addShape(const Shape& shape);
    void show() const;
    bool isEmpty() const;
};