#pragma once
#include "Shape.hpp"
#include <vector>

class Slide {
private:
    std::vector<Shape> shapes;

public:
    void addShape(Shape s);
    std::vector<Shape>& getShapes();
    const std::vector<Shape>& getShapes() const;
    bool isEmpty() const;
};