#pragma once

#include <string>
#include <iostream>

class Shape {
private:
    std::string name;
    int x;
    int y;

public:
    Shape(const std::string& name, int x, int y);

    void draw() const;
};