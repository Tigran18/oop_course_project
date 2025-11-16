#pragma once
#include <string>
#include <vector>

class Shape {
private:
    std::string name;
    int x, y;
    std::vector<uint8_t> imageData;
    bool isImageFlag = false;

public:
    Shape(std::string n, int px, int py);
    Shape(std::string n, int px, int py, std::vector<uint8_t> data);

    const std::string& getName() const;
    int getX() const;
    int getY() const;
    const std::vector<uint8_t>& getImageData() const;
    bool isImage() const;
};