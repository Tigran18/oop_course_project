#pragma once
#include "Shape.hpp"
#include <vector>

class Slide {
private:
    std::vector<Shape> shapes;

public:
    void addShape(const Shape& shape);
    void show() const;
    bool isEmpty() const;

    const std::vector<Shape>& getShapes() const;
    std::vector<Shape>& getShapes();
};