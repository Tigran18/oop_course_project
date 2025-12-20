#include "Commands.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace fs = std::filesystem;

void CommandExit::execute() {

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

    // Optional: auto-append .pptx if user didn't type it
    // (uncomment if you want this behavior)
    // if (name.size() < 5 || name.substr(name.size() - 5) != ".pptx")
    //     name += ".pptx";

    auto& slideshows = ctrl.getSlideshows();
    auto& order      = ctrl.getPresentationOrder();
    auto& index      = ctrl.getPresentationIndex();

    // Fast duplicate check using the index map
    if (index.find(name) != index.end()) {
        std::cout << "[ERR] Presentation '" << name << "' already exists\n";
        return;
    }

    slideshows.emplace_back(name);
    const size_t newIdx = slideshows.size() - 1;

    index[name] = newIdx;
    order.push_back(name);

    ctrl.getCurrentIndex() = newIdx;

    std::cout << "[INFO] Created presentation: " << name << "\n";
}


void CommandOpen::execute() {
    if (args.empty()) {
        std::cout << "[ERR] Usage: open <file.pptx>\n";
        return;
    }

    std::string path = args[0];
    if (!utils::endsWith(path, ".pptx"))
        path += ".pptx";

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

void CommandSave::execute() {
    auto& ctrl = Controller::instance();

    if (ctrl.getSlideshows().empty()) {
        error() << "No presentations to save.\n";
        return;
    }

    std::string out = filename;

    // Ensure .pptx extension
    if (out.size() < 5 || out.substr(out.size() - 5) != ".pptx")
        out += ".pptx";

    bool ok = PPTXSerializer::save(
        ctrl.getSlideshows(),        // ALL slideshows
        ctrl.getPresentationOrder(), // CORRECT ORDER
        out
    );

    if (ok)
        success() << "Saved to " << out << "\n";
    else
        error() << "Failed to save " << out << "\n";
}


void CommandAutoSave::execute() {
    auto& ctrl = Controller::instance();
    if (args.empty()) {
        std::cout << "[INFO] Autosave is currently "
                  << (ctrl.getAutoSaveOnExit() ? "ON\n" : "OFF\n");
        return;
    }

    if (args[0] == "on") {
        ctrl.setAutoSaveOnExit(true);
        std::cout << "[INFO] Autosave on exit: ON\n";
    } else if (args[0] == "off") {
        ctrl.setAutoSaveOnExit(false);
        std::cout << "[INFO] Autosave on exit: OFF\n";
    } else {
        std::cout << "[ERR] autosave expects 'on' or 'off'\n";
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
}

void CommandPreview::execute() {
    auto& ss = ctrl.getCurrentSlideshow();
    auto& slides = ss.getSlides();

    if (slides.empty()) {
        error() << "No slides.\n";
        return;
    }

    const Slide& slide = slides[ss.getCurrentIndex()];

    for (const auto& sh : slide.getShapes()) {
        if (sh.isImage()) {
            const auto& data = sh.getImageData();

            std::vector<unsigned char> rgba;
            unsigned w = 0, h = 0;

            unsigned err = lodepng::decode(rgba, w, h, data.data(), data.size());
            if (err) {
                error() << "PNG decode error.\n";
                return;
            }

            // Render in full color!
            utils::renderColorPreview(rgba, w, h, 120);
            return;
        }
    }

    error() << "No image on this slide.\n";
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