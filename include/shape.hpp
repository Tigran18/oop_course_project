#pragma once
#include <string>

class Shape {
private:
    std::string name;
    int x, y;
public:
    Shape(const std::string& n, int xx, int yy);

    void draw() const;

    const std::string& getName() const;

    int getX() const;

    int getY() const;
};