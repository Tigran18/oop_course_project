#pragma once
#include <string>
#include <vector>
#include <cstdint>

enum class ShapeKind {
    Text,
    Image,
    Rect,
    Ellipse
};

class Shape {
private:
    std::string name_;
    std::string text_;

    int x_ = 0;
    int y_ = 0;

    int w_ = 220;
    int h_ = 80;

    std::vector<uint8_t> imageData_;
    ShapeKind kind_ = ShapeKind::Text;

private:
    void parseSpecialSyntax(const std::string& raw);

public:
    Shape(std::string n, int px, int py);
    Shape(std::string n, int px, int py, std::vector<uint8_t> data);

    const std::string& getName() const;
    int getX() const;
    int getY() const;
    const std::vector<uint8_t>& getImageData() const;
    bool isImage() const;

    ShapeKind kind() const;
    const std::string& getText() const;
    int getW() const;
    int getH() const;

    void setPos(int px, int py);
    void setSize(int w, int h);
    void setText(std::string t);
};
