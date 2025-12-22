#include "Slide.hpp"

void Slide::addShape(const Shape& s)
{
    shapes_.push_back(s);
}

void Slide::addShape(Shape&& s)
{
    shapes_.push_back(std::move(s));
}

bool Slide::isEmpty() const
{
    return shapes_.empty();
}

std::vector<Shape>& Slide::getShapes()
{
    return shapes_;
}

const std::vector<Shape>& Slide::getShapes() const
{
    return shapes_;
}
