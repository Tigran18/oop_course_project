#pragma once

#include <vector>
#include "Shape.hpp"

class Slide
{
public:
    void addShape(const Shape& s);
    void addShape(Shape&& s);

    bool isEmpty() const;

    std::vector<Shape>& getShapes();
    const std::vector<Shape>& getShapes() const;

private:
    std::vector<Shape> shapes_;
};
