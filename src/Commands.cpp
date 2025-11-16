#include "../include/Commands.hpp"
#include "../include/Controller.hpp"
#include "../include/Serializer.hpp"
#include "../include/Functions.hpp"
#include <iostream>
#include <cstdlib>

void CommandExit::execute() {
    std::cout << "[INFO] Exiting slideshow\n";
    std::exit(EXIT_SUCCESS);
}

void CommandEmpty::execute() {

}

void CommandHelp::execute() {
    std::cout << "Available  s:\n"
              << "  next                  - Go to next slide\n"
              << "  prev                  - Go to previous slide\n"
              << "  show                  - Show current slide\n"
              << "  nextfile              - Next presentation\n"
              << "  prevfile              - Previous presentation\n"
              << "  goto <n>              - Go to slide n in current file\n"
              << "  goto <file> <n>       - Go to slide n in file\n"
              << "  add slide <name> <x> <y>     - Add shape to current slide\n"
              << "  add slideshow <path>         - Add or create presentation\n"
              << "  save <file.pptx>             - Save all presentations\n"
              << "  load <file.pptx>             - Load presentations\n"
              << "  help                  - Show this help\n"
              << "  exit                  - Exit program\n";
}

void CommandNextFile::execute() {
    if (currentIndex + 1 < slideshows.size()) {
        ++currentIndex;
        slideshows[currentIndex].show();
    } 
    else {
        std::cout << "[WARN] Already at last presentation\n";
    }
}

void CommandPrevFile ::execute() {
    if (currentIndex > 0) {
        --currentIndex;
        slideshows[currentIndex].show();
    } 
    else {
        std::cout << "[WARN] Already at first presentation\n";
    }
}

CommandGoto::CommandGoto (size_t& idx,
    std::vector<SlideShow>& ss,
    std::map<std::string, size_t>& pIndex,
    std::vector<std::string>& pOrder,
    const std::vector<std::string>& arguments)
    : currentIndex(idx), slideshows(ss), presentationIndex(pIndex),
      presentationOrder(pOrder), args(arguments) {

      }

void CommandGoto ::execute() {
    if (args.empty()) { 
        std::cout << "[ERR] Goto requires at least a slide number\n"; 
        return; 
    }
    if (args.size() == 1) {
        try { 
            size_t n = std::stoul(args[0]); 
            slideshows[currentIndex].gotoSlide(n); 
        }
        catch (...) { 
            std::cout << "[ERR] Invalid slide number: " << args[0] << "\n"; 
        }
    } 
    else if (args.size() == 2) {
        std::string path = utils::normalizePath(args[0]);
        auto it = presentationIndex.find(path);
        if (it == presentationIndex.end()) {
            std::cout << "[ERR] Presentation not found: " << args[0] << "\n";
            std::cout << "Available: "; 
            for (const auto& n : presentationOrder) {
                std::cout << n << " "; std::cout << "\n";
            }
            return;
        }
        try { 
            size_t n = std::stoul(args[1]); 
            currentIndex = it->second; 
            slideshows[currentIndex].gotoSlide(n); 
        }
        catch (...) { 
            std::cout << "[ERR] Invalid slide number: " << args[1] << "\n"; 
        }
    } 
    else {
        std::cout << "[ERR] Usage: goto [filename] <slide_number>\n";
    }
}

void CommandSave ::execute() {
    if (args.empty()) {
        std::cout << "[ERR] Usage: save <filename.pptx>\n";
        return;
    }
    std::string userName = utils::normalizePath(args[0]);
    std::string binPath  = userName + ".bin";
    if (Serializer::save(ctrl.getSlideshows(), ctrl.getPresentationOrder(), binPath)) {
        std::cout << "[INFO] Saved to " << binPath << "\n";
    } 
    else {
        std::cout << "[ERR] Failed to save: " << binPath << "\n";
    }
}

void CommandLoad ::execute() {
    if (args.empty()) {
        std::cout << "[ERR] Usage: load <filename.pptx>\n";
        return;
    }
    std::string userName = utils::normalizePath(args[0]);
    std::string binPath  = userName + ".bin";
    size_t oldIdx = ctrl.getCurrentIndex();
    if (Serializer::load(ctrl.getSlideshows(), ctrl.getPresentationIndex(),
                         ctrl.getPresentationOrder(), ctrl.getCurrentIndex(), binPath)) {
        std::cout << "[INFO] Loaded " << ctrl.getSlideshows().size() << " presentation(s)\n";
        if (!ctrl.getSlideshows().empty()) {
            ctrl.getSlideshows()[ctrl.getCurrentIndex()].show();
        }
    } 
    else {
        std::cout << "[ERR] Failed to load: " << binPath << "\n";
        ctrl.getCurrentIndex() = oldIdx;
    }
}


void CommandAdd::execute() {
    if (args.empty() || (args[0] != "slide" && args[0] != "slideshow")) {
        std::cout << "[ERR] Usage: add slide <name> <x> <y> | add slideshow <path>\n";
        return;
    }
    if (args[0] == "slideshow" && args.size() == 2) {
        std::string path = utils::normalizePath(args[1]);
        SlideShow ss(path);
        ctrl.getSlideshows().push_back(std::move(ss));
        size_t idx = ctrl.getSlideshows().size() - 1;
        ctrl.getPresentationIndex()[path] = idx;
        ctrl.getPresentationOrder().push_back(path);
        ctrl.getCurrentIndex() = idx;
        std::cout << "[INFO] Created new in-memory presentation: " << path << "\n";
        return;
    }
    if (args[0] == "slide" && args.size() == 4) {
        if (ctrl.getSlideshows().empty()) {
            std::cout << "[ERR] No presentation. Use 'add slideshow <name>' first.\n";
            return;
        }
        try {
            std::string name = args[1];
            int x = std::stoi(args[2]);
            int y = std::stoi(args[3]);
            auto& curr = ctrl.getSlideshows()[ctrl.getCurrentIndex()];
            if (curr.getSlides().empty()) curr.getSlides().emplace_back();
            curr.getSlides().back().addShape(Shape(name, x, y));
            std::cout << "[INFO] Added shape '" << name << "' at (" << x << "," << y << ")\n";
        } 
        catch (...) {
            std::cout << "[ERR] Invalid shape parameters\n";
        }
        return;
    }
    std::cout << "[ERR] Invalid add command\n";
}