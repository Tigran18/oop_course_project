#include "../include/Serializer.hpp"
#include <fstream>
#include <iostream>

bool Serializer::save(const std::vector<SlideShow>& slideshows,
                      const std::vector<std::string>& order,
                      const std::string& filepath) {
    std::ofstream out(filepath, std::ios::binary);
    if (!out) {
        return false;
    }
    uint32_t count = static_cast<uint32_t>(slideshows.size());
    out.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (size_t i = 0; i < slideshows.size(); ++i) {
        const auto& ss   = slideshows[i];
        const auto& name = order[i];
        uint16_t len = static_cast<uint16_t>(name.size());
        out.write(reinterpret_cast<const char*>(&len), sizeof(len));
        out.write(name.c_str(), len);
        uint32_t curIdx = static_cast<uint32_t>(ss.getCurrentIndex());
        out.write(reinterpret_cast<const char*>(&curIdx), sizeof(curIdx));
        uint32_t slideCount = static_cast<uint32_t>(ss.getSlides().size());
        out.write(reinterpret_cast<const char*>(&slideCount), sizeof(slideCount));
        for (const auto& slide : ss.getSlides()) {
            uint32_t shapeCount = static_cast<uint32_t>(slide.getShapes().size());
            out.write(reinterpret_cast<const char*>(&shapeCount), sizeof(shapeCount));
            for (const auto& shape : slide.getShapes()) {
                uint16_t nlen = static_cast<uint16_t>(shape.getName().size());
                out.write(reinterpret_cast<const char*>(&nlen), sizeof(nlen));
                out.write(shape.getName().c_str(), nlen);
                int x = shape.getX(), y = shape.getY();
                out.write(reinterpret_cast<const char*>(&x), sizeof(x));
                out.write(reinterpret_cast<const char*>(&y), sizeof(y));
            }
        }
    }
    return true;
}

bool Serializer::load(std::vector<SlideShow>& slideshows,
                      std::map<std::string, size_t>& index,
                      std::vector<std::string>& order,
                      size_t& currentIndex,
                      const std::string& filepath) {
    std::ifstream in(filepath, std::ios::binary);
    if (!in) {
        return false;
    }
    slideshows.clear(); 
    index.clear(); 
    order.clear();
    uint32_t count;
    if (!in.read(reinterpret_cast<char*>(&count), sizeof(count))) {
        return false;
    }
    for (uint32_t i = 0; i < count; ++i) {
        uint16_t len;
        if (!in.read(reinterpret_cast<char*>(&len), sizeof(len))) {
            return false;
        }
        std::string name(len, '\0');
        if (!in.read(&name[0], len)) {
            return false;
        }
        uint32_t curIdx;
        if (!in.read(reinterpret_cast<char*>(&curIdx), sizeof(curIdx))) {
            return false;
        }
        SlideShow ss(name);
        ss.setCurrentIndex(curIdx);
        uint32_t slideCount;
        if (!in.read(reinterpret_cast<char*>(&slideCount), sizeof(slideCount))) {
            return false;
        }
        for (uint32_t s = 0; s < slideCount; ++s) {
            Slide slide;
            uint32_t shapeCount;
            if (!in.read(reinterpret_cast<char*>(&shapeCount), sizeof(shapeCount))) {
                return false;
            }
            for (uint32_t h = 0; h < shapeCount; ++h) {
                uint16_t nlen;
                if (!in.read(reinterpret_cast<char*>(&nlen), sizeof(nlen))) {
                    return false;
                }
                std::string sname(nlen, '\0');
                if (!in.read(&sname[0], nlen)) {
                    return false;
                }
                int x, y;
                if (!in.read(reinterpret_cast<char*>(&x), sizeof(x))) {
                    return false;
                }
                if (!in.read(reinterpret_cast<char*>(&y), sizeof(y))) {
                    return false;
                }
                slide.addShape(Shape(sname, x, y));
            }
            ss.getSlides().push_back(std::move(slide));
        }
        slideshows.push_back(std::move(ss));
        index[name] = i;
        order.push_back(name);
    }
    currentIndex = slideshows.empty() ? 0 : 0;
    return true;
}