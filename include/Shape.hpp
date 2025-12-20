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

class Shape
{
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
    Shape(std::string n, int px, int py, ShapeKind k, int w, int h);

    const std::string& getName() const;
    const std::string& getText() const;

    int getX() const;
    int getY() const;
    int getW() const;
    int getH() const;

    void setX(int v);
    void setY(int v);
    void setW(int v);
    void setH(int v);
    void setText(const std::string& t);

    ShapeKind kind() const;

    const std::vector<uint8_t>& getImageData() const;
    bool isImage() const;
};
