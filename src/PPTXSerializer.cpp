#include "../include/PPTXSerializer.hpp"
#include "../include/SlideShow.hpp"
#include "../include/Functions.hpp"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

bool PPTXSerializer::save(const std::vector<SlideShow>& slideshows,
                          const std::vector<std::string>& order,
                          const std::string& pptxPath) {
    std::string path = pptxPath;
    if (!utils::endsWith(path, ".pptx")) {
        path += ".pptx";
    }
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return false;
    }
    out << "PPTX_STUB_V1\n";
    out << slideshows.size() << "\n";
    for (const auto& ss : slideshows) {
        out << ss.getFilename() << "\n";
        out << ss.getCurrentIndex() << "\n";
        out << ss.getSlides().size() << "\n";
        for (const auto& slide : ss.getSlides()) {
            out << slide.getShapes().size() << "\n";
            for (const auto& shape : slide.getShapes()) {
                out << (shape.isImage() ? "IMAGE" : "TEXT") << "\n";
                out << shape.getName() << "\n";
                out << shape.getX() << "\n";
                out << shape.getY() << "\n";
                if (shape.isImage()) {
                    out << shape.getImageData().size() << "\n";
                    out.write(reinterpret_cast<const char*>(shape.getImageData().data()), shape.getImageData().size());
                }
            }
        }
    }
    out << "---ORDER---\n";
    for (const auto& name : order) {
        out << name << "\n";
    }
    return true;
}

LoadResult PPTXSerializer::load(std::vector<SlideShow>& slideshows,
                                std::map<std::string, size_t>& index,
                                std::vector<std::string>& order,
                                size_t& currentIdx,
                                const std::string& pptxPath) {
    std::string path = pptxPath;
    if (!utils::endsWith(path, ".pptx")) {
        path += ".pptx";
    }
    if (!fs::exists(path)) {
        return LoadResult::NOT_FOUND;
    }
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return LoadResult::IO_ERROR;
    }
    std::string line;
    if (!std::getline(in, line) || line != "PPTX_STUB_V1") {
        return LoadResult::CORRUPTED;
    }
    if (!std::getline(in, line)) {
        return LoadResult::CORRUPTED;
    }
    size_t count = std::stoull(line);
    slideshows.clear();
    slideshows.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        std::string filename, curIdxStr, slideCountStr;
        if (!std::getline(in, filename) || !std::getline(in, curIdxStr) || !std::getline(in, slideCountStr)) {
            return LoadResult::CORRUPTED;
        }
        SlideShow ss(filename);
        ss.setCurrentIndex(std::stoull(curIdxStr));
        size_t slideCount = std::stoull(slideCountStr);
        for (size_t s = 0; s < slideCount; ++s) {
            std::string shapeCountStr;
            if (!std::getline(in, shapeCountStr)) {
                return LoadResult::CORRUPTED;
            }
            size_t shapeCount = std::stoull(shapeCountStr);
            Slide slide;
            for (size_t h = 0; h < shapeCount; ++h) {
                std::string type, name, xStr, yStr;
                if (!std::getline(in, type) || !std::getline(in, name) || !std::getline(in, xStr) || !std::getline(in, yStr)) {
                    return LoadResult::CORRUPTED;
                }
                int x = std::stoi(xStr);
                int y = std::stoi(yStr);
                if (type == "IMAGE") {
                    std::string sizeStr;
                    if (!std::getline(in, sizeStr)) return LoadResult::CORRUPTED;
                    size_t size = std::stoull(sizeStr);
                    std::vector<uint8_t> data(size);
                    in.read(reinterpret_cast<char*>(data.data()), size);
                    slide.addShape(Shape(name, x, y, std::move(data)));
                } 
                else {
                    slide.addShape(Shape(name, x, y));
                }
            }
            ss.getSlides().push_back(std::move(slide));
        }
        slideshows.push_back(std::move(ss));
    }
    if (!std::getline(in, line) || line != "---ORDER---") {
        return LoadResult::CORRUPTED;
    }
    order.clear();
    while (std::getline(in, line) && !line.empty()) {
        order.push_back(line);
    }
    index.clear();
    for (size_t i = 0; i < slideshows.size(); ++i) {
        index[slideshows[i].getFilename()] = i;
    }
    currentIdx = order.empty() ? 0 : index[order.back()];
    return LoadResult::SUCCESS;
}