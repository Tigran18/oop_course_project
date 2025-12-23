#include "../include/Shape.hpp"

#include <algorithm>
#include <cctype>
#include <string>

static std::string toLowerCopy(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

static std::string trimCopy(const std::string& s)
{
    size_t a = 0;
    while (a < s.size() && std::isspace(static_cast<unsigned char>(s[a]))) ++a;

    size_t b = s.size();
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;

    return s.substr(a, b - a);
}

static bool startsWith(const std::string& s, const std::string& pref)
{
    return s.size() >= pref.size() && s.compare(0, pref.size(), pref) == 0;
}

static bool parseIntStrict(const std::string& s, int& out)
{
    if (s.empty()) return false;
    for (char c : s) {
        if (!std::isdigit(static_cast<unsigned char>(c)) && c != '-' && c != '+')
            return false;
    }
    try {
        out = std::stoi(s);
        return true;
    } catch (...) {
        return false;
    }
}

Shape::Shape(std::string n, int px, int py)
{
    x_ = px;
    y_ = py;

    kind_ = ShapeKind::Text;
    w_ = 220;
    h_ = 80;

    parseSpecialSyntax(n);

    if (name_.empty()) name_ = text_;
    if (text_.empty()) text_ = name_;
}

Shape::Shape(std::string n, int px, int py, std::vector<uint8_t> data)
{
    name_ = std::move(n);
    text_ = name_;
    x_ = px;
    y_ = py;
    imageData_ = std::move(data);
    kind_ = ShapeKind::Image;
    w_ = 1;
    h_ = 1;
}

Shape::Shape(std::string n, int px, int py, ShapeKind k, int w, int h)
{
    name_ = std::move(n);
    text_ = name_;
    x_ = px;
    y_ = py;
    kind_ = k;
    w_ = (w > 0) ? w : 220;
    h_ = (h > 0) ? h : 80;
}

void Shape::parseSpecialSyntax(const std::string& raw)
{
    std::string s = trimCopy(raw);
    std::string low = toLowerCopy(s);

    bool isRect = startsWith(low, "rect");
    bool isEllipse = startsWith(low, "ellipse");

    if (!isRect && !isEllipse) {
        kind_ = ShapeKind::Text;
        name_ = s;
        text_ = s;
        return;
    }

    kind_ = isRect ? ShapeKind::Rect : ShapeKind::Ellipse;
    size_t pos = isRect ? 4 : 7;

    int W = 220, H = 80;
    if (pos < s.size() && s[pos] == '(') {
        size_t close = s.find(')', pos);
        if (close != std::string::npos) {
            std::string inside = s.substr(pos + 1, close - (pos + 1));
            size_t comma = inside.find(',');
            if (comma != std::string::npos) {
                std::string ws = trimCopy(inside.substr(0, comma));
                std::string hs = trimCopy(inside.substr(comma + 1));
                int wTmp = 0, hTmp = 0;
                if (parseIntStrict(ws, wTmp) && parseIntStrict(hs, hTmp)) {
                    if (wTmp > 0) W = wTmp;
                    if (hTmp > 0) H = hTmp;
                }
            }
            pos = close + 1;
        }
    }

    std::string textPart;
    size_t colon = s.find(':', pos);
    if (colon != std::string::npos) {
        textPart = trimCopy(s.substr(colon + 1));
    } else {
        if (pos < s.size())
            textPart = trimCopy(s.substr(pos));
    }

    w_ = W;
    h_ = H;

    text_ = textPart;
    name_ = text_.empty() ? std::string(isRect ? "Rect" : "Ellipse") : text_;
}

const std::string& Shape::getName() const { return name_; }
const std::string& Shape::getText() const { return text_; }

int Shape::getX() const { return x_; }
int Shape::getY() const { return y_; }
int Shape::getW() const { return w_; }
int Shape::getH() const { return h_; }

void Shape::setX(int v) { x_ = v; }
void Shape::setY(int v) { y_ = v; }
void Shape::setW(int v) { if (v > 0) w_ = v; }
void Shape::setH(int v) { if (v > 0) h_ = v; }

void Shape::setText(const std::string& t)
{
    text_ = t;
    if (kind_ != ShapeKind::Image) {
        if (!t.empty()) name_ = t;
    }
}

ShapeKind Shape::kind() const { return kind_; }

const std::vector<uint8_t>& Shape::getImageData() const { return imageData_; }
bool Shape::isImage() const { return kind_ == ShapeKind::Image; }

static int clampCropPct(int v)
{
    if (v < 0) return 0;
    if (v > 100000) return 100000;
    return v;
}

int Shape::getCropL() const { return cropL_; }
int Shape::getCropT() const { return cropT_; }
int Shape::getCropR() const { return cropR_; }
int Shape::getCropB() const { return cropB_; }

void Shape::setCrop(int l, int t, int r, int b)
{
    cropL_ = clampCropPct(l);
    cropT_ = clampCropPct(t);
    cropR_ = clampCropPct(r);
    cropB_ = clampCropPct(b);

    // Prevent invalid crop that removes the whole image.
    if (cropL_ + cropR_ > 99999) cropR_ = std::max(0, 99999 - cropL_);
    if (cropT_ + cropB_ > 99999) cropB_ = std::max(0, 99999 - cropT_);
}

