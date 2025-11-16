#include "../include/Commands.hpp"
#include "../include/PPTXSerializer.hpp"
#include "../include/Slide.hpp"
#include "../include/Functions.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

namespace fs = std::filesystem;

void CommandExit::execute() {
    auto& ctrl = Controller::instance();
    if (ctrl.getAutoSaveOnExit() && !ctrl.getSlideshows().empty()) {
        PPTXSerializer::save(ctrl.getSlideshows(), ctrl.getPresentationOrder(), "autosave.pptx");
        std::cout << "[INFO] Autosaved to autosave.pptx\n";
    }
    std::exit(0);
}

void CommandSave::execute() {
    std::string path = args.empty() ? "session.pptx" : args[0];
    if (!utils::endsWith(path, ".pptx")) path += ".pptx";
    auto& ctrl = Controller::instance();
    if (PPTXSerializer::save(ctrl.getSlideshows(), ctrl.getPresentationOrder(), path)) {
        std::cout << "[INFO] Session saved to " << path << "\n";
    } else {
        std::cout << "[ERR] Failed to save: " << path << "\n";
    }
}

void CommandHelp::execute() {
    std::cout << "=== SLIDESHOW COMMANDS ===\n"
              << "  create slideshow <name> - create new empty presentation\n"
              << "  open <file.pptx>       - open existing PPTX file\n"
              << "  save <name.pptx>       - save current session\n"
              << "  autosave [on|off]      - toggle autosave on exit\n"
              << "\n=== SLIDE COMMANDS ===\n"
              << "  add slide <name> <x> <y>           - add text shape\n"
              << "  add slide image <name> <x> <y> \"<path>\" - add image\n"
              << "  remove <n>               - delete slide n\n"
              << "  move <from> <to>         - reorder slides\n"
              << "  goto <n>                 - go to slide n\n"
              << "  next / prev / show       - navigate current slide\n"
              << "  nextfile / prevfile      - switch presentation\n"
              << "  preview                  - extract current image to file\n"
              << "\n=== HISTORY ===\n"
              << "  undo / redo              - undo/redo changes\n"
              << "\n=== OTHER ===\n"
              << "  help                     - this help\n"
              << "  exit                     - quit (with autosave if enabled)\n";
}

void CommandCreateSlideshow::execute() {
    if (args.empty()) {
        std::cout << "[ERR] Usage: create slideshow <name>\n";
        return;
    }
    std::string name = args[0];
    for (const auto& ss : ctrl.getSlideshows()) {
        if (ss.getFilename() == name) {
            std::cout << "[ERR] Presentation '" << name << "' already exists\n";
            return;
        }
    }

    SlideShow ss(name);
    auto& slideshows = ctrl.getSlideshows();
    auto& order = ctrl.getPresentationOrder();
    auto& index = ctrl.getPresentationIndex();
    slideshows.push_back(std::move(ss));
    size_t idx = slideshows.size() - 1;
    index[name] = idx;
    order.push_back(name);
    ctrl.getCurrentIndex() = idx;
    std::cout << "[INFO] Created presentation: " << name << "\n";
}

void CommandOpen::execute() {
    if (args.empty()) {
        std::cout << "[ERR] Usage: open <file.pptx>\n";
        return;
    }
    std::string path = args[0];
    if (!utils::endsWith(path, ".pptx")) path += ".pptx";

    auto& ctrl = Controller::instance();
    size_t dummy = 0;
    if (!PPTXSerializer::load(ctrl.getSlideshows(),
                              ctrl.getPresentationIndex(),
                              ctrl.getPresentationOrder(),
                              dummy,
                              path)) {
        std::cout << "[ERR] Cannot load file: " << path << "\n";
        return;
    }
    ctrl.getCurrentIndex() = dummy;
    std::cout << "[INFO] Loaded session from " << path << "\n";
}

void CommandAutoSave::execute() {
    auto& ctrl = Controller::instance();
    if (args.empty()) {
        std::cout << "[INFO] Autosave on exit: " << (ctrl.getAutoSaveOnExit() ? "on" : "off") << "\n";
        return;
    }
    std::string arg = args[0];
    std::transform(arg.begin(), arg.end(), arg.begin(), ::tolower);
    if (arg == "on") {
        ctrl.setAutoSaveOnExit(true);
        std::cout << "[INFO] Autosave enabled\n";
    } 
    else if (arg == "off") {
        ctrl.setAutoSaveOnExit(false);
        std::cout << "[INFO] Autosave disabled\n";
    } 
    else {
        std::cout << "[ERR] Usage: autosave [on|off]\n";
    }
}

void CommandAddSlide::execute() {
    bool isImage = false;
    std::string name, path;
    int x = 0, y = 0;
    std::vector<uint8_t> imgData;
    if (args.size() == 3) {
        name = args[0];
        x = std::stoi(args[1]);
        y = std::stoi(args[2]);
    } 
    else if (args.size() == 5 && args[0] == "image") {
        isImage = true;
        name = args[1];
        x = std::stoi(args[2]);
        y = std::stoi(args[3]);
        path = args[4];
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            std::cout << "[ERR] Cannot read image: " << path << "\n";
            return;
        }
        imgData = std::vector<uint8_t>(std::istreambuf_iterator<char>(file), {});
    } 
    else {
        std::cout << "[ERR] Usage:\n"
                  << "  add slide <name> <x> <y>\n"
                  << "  add slide image <name> <x> <y> \"<path>\"\n";
        return;
    }
    Slide slide;
    if (isImage) {
        slide.addShape(Shape(name, x, y, std::move(imgData)));
    }
    else {
        slide.addShape(Shape(name, x, y));
    }
    auto& ss = ctrl.getCurrentSlideshow();
    ss.getSlides().push_back(std::move(slide));
    std::cout << "[INFO] Added " << (isImage ? "image" : "text") << " slide: " << name << "\n";
}

void CommandRemoveSlide::execute() {
    if (args.empty()) { 
        std::cout << "[ERR] Usage: remove <n>\n"; 
        return; 
    }
    size_t n = std::stoul(args[0]) - 1;
    auto& slides = ctrl.getCurrentSlideshow().getSlides();
    if (n >= slides.size()) { 
        std::cout << "[ERR] Invalid slide number\n"; 
        return; 
    }
    slides.erase(slides.begin() + n);
    std::cout << "[INFO] Removed slide " << (n + 1) << "\n";
}

void CommandMoveSlide::execute() {
    if (args.size() < 2) { 
        std::cout << "[ERR] Usage: move <from> <to>\n"; 
        return; 
    }
    size_t from = std::stoul(args[0]) - 1;
    size_t to = std::stoul(args[1]) - 1;
    auto& slides = ctrl.getCurrentSlideshow().getSlides();
    if (from >= slides.size() || to >= slides.size()) { 
        std::cout << "[ERR] Invalid slide number\n"; 
        return; 
    }
    auto it = slides.begin() + from;
    slides.insert(slides.begin() + to + (to >= from ? 1 : 0), std::move(*it));
    slides.erase(slides.begin() + (to >= from ? from + 1 : from));
    std::cout << "[INFO] Moved slide " << (from + 1) << " to " << (to + 1) << "\n";
}

void CommandGotoSlide::execute() {
    if (args.empty()) { 
        std::cout << "[ERR] Usage: goto <n>\n"; 
        return; 
    }
    size_t n = std::stoul(args[0]) - 1;
    auto& ss = ctrl.getCurrentSlideshow();
    if (n >= ss.getSlides().size()) { 
        std::cout << "[ERR] Invalid slide number\n"; 
        return; 
    }
    ss.setCurrentIndex(n);
    ss.show();
}

void CommandNext::execute() { 
    ss.next(); 
    ss.show(); 
}

void CommandPrev::execute() { 
    ss.prev(); 
    ss.show(); 
}

void CommandShow::execute() {
    ss.show(); 
    const auto& slide = ss.getSlides()[ss.getCurrentIndex()];
    const auto& shapes = slide.getShapes();
    const Shape* imgShape = nullptr;
    for (const auto& s : shapes) {
        if (s.isImage()) {
            imgShape = &s;
            break;
        }
    }
    if (!imgShape) {
        return;
    }
    const auto& data = imgShape->getImageData();
    std::cout << "[PREVIEW] Image '" << imgShape->getName() << "' (" << data.size() << " bytes)\n";
    int width, height, channels;
    unsigned char* img = stbi_load_from_memory(
        data.data(),
        static_cast<int>(data.size()),
        &width, &height, &channels, 3
    );
    if (!img) {
        std::cout << "[ERR] Failed to decode image.\n";
        return;
    }
    const char* gradient = "@%#*+=-:. ";
    const int maxWidth = 80;
    const int maxHeight = 40;
    int newWidth = width;
    int newHeight = height;
    float aspectRatio = float(width) / float(height);
    if (width > maxWidth) {
        newWidth = maxWidth;
        newHeight = static_cast<int>(maxWidth / aspectRatio);
    }
    if (newHeight > maxHeight) {
        newHeight = maxHeight;
        newWidth = static_cast<int>(maxHeight * aspectRatio);
    }
    auto getPixel = [&](int x, int y, int c) -> unsigned char {
        int sx = x * width / newWidth;
        int sy = y * height / newHeight;
        return img[(sy * width + sx) * 3 + c];
    };
    for (int y = 0; y < newHeight; ++y) {
        for (int x = 0; x < newWidth; ++x) {
            unsigned char r = getPixel(x, y, 0);
            unsigned char g = getPixel(x, y, 1);
            unsigned char b = getPixel(x, y, 2);
            float gray = 0.2126f * r + 0.7152f * g + 0.0722f * b; // luminance
            int index = static_cast<int>(gray / 256.0f * 10);
            if (index > 9) index = 9;
            std::cout << gradient[index];
        }
        std::cout << "\n";
    }
    stbi_image_free(img);
    std::cout << "[INFO] ASCII image preview complete.\n";
}

void CommandNextFile::execute() {
    auto& order = ctrl.getPresentationOrder();
    if (order.empty()) {
        return;
    }
    size_t& idx = ctrl.getCurrentIndex();
    idx = (idx + 1) % order.size();
    std::cout << "[INFO] Switched to: " << ctrl.getCurrentSlideshow().getFilename() << "\n";
    ctrl.getCurrentSlideshow().show();
}

void CommandPrevFile::execute() {
    auto& order = ctrl.getPresentationOrder();
    if (order.empty()) {
        return;
    }
    size_t& idx = ctrl.getCurrentIndex();
    idx = (idx == 0) ? order.size() - 1 : idx - 1;
    std::cout << "[INFO] Switched to: " << ctrl.getCurrentSlideshow().getFilename() << "\n";
    ctrl.getCurrentSlideshow().show();
}

void CommandUndo::execute() {
    if (ctrl.undo()) {
        std::cout << "[INFO] Undo successful\n";
    }
    else {
        std::cout << "[ERR] Nothing to undo\n";
    }
}

void CommandRedo::execute() {
    if (ctrl.redo()) {
        std::cout << "[INFO] Redo successful\n";
    }
    else {
        std::cout << "[ERR] Nothing to redo\n";
    }
}