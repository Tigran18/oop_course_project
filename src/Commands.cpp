#include "Commands.hpp"

#include "Controller.hpp"
#include "SlideShow.hpp"
#include "Slide.hpp"
#include "Shape.hpp"
#include "Color.hpp"

#include "PPTXSerializer.hpp"
#include "Functions.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <algorithm>
#include <iomanip>
#include <cctype>

#include "lodepng.h"

// stb_image for converting any image -> PNG bytes (so PPTX always contains real PNG)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace {

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return (unsigned char)std::tolower(c); });
    return s;
}

bool parseInt(const std::string& s, int& out) {
    try {
        size_t pos = 0;
        int v = std::stoi(s, &pos);
        if (pos != s.size()) return false;
        out = v;
        return true;
    } catch (...) {
        return false;
    }
}

std::string joinFrom(const std::vector<std::string>& a, size_t i) {
    std::string r;
    for (size_t k = i; k < a.size(); ++k) {
        if (k > i) r += " ";
        r += a[k];
    }
    return r;
}

bool ensureCurrentSlide(Controller& ctrl, SlideShow*& outSS, Slide*& outSlide) {
    if (ctrl.getSlideshows().empty()) {
        error() << "No presentation loaded. Use 'create slideshow <name>' or 'open <file.pptx>' first.\n";
        return false;
    }
    SlideShow& ss = ctrl.getCurrentSlideshow();
    if (ss.getSlides().empty()) {
        error() << "No slides. Use 'add slide' first.\n";
        return false;
    }
    outSS = &ss;
    outSlide = &ss.getSlides()[ss.getCurrentIndex()];
    return true;
}

bool loadAnyImageAsPngBytes(const std::string& path,
                            std::vector<uint8_t>& outPng,
                            int& outW,
                            int& outH)
{
    int w = 0, h = 0, comp = 0;
    unsigned char* rgba = stbi_load(path.c_str(), &w, &h, &comp, 4);
    if (!rgba) {
        error() << "Failed to load image: " << path << "\n";
        return false;
    }

    std::vector<unsigned char> rgbaVec(rgba, rgba + (size_t)w * (size_t)h * 4);
    stbi_image_free(rgba);

    std::vector<unsigned char> png;
    unsigned err = lodepng::encode(png, rgbaVec, (unsigned)w, (unsigned)h);
    if (err) {
        error() << "Failed to encode PNG: " << lodepng_error_text(err) << "\n";
        return false;
    }

    outPng.assign(png.begin(), png.end());
    outW = w;
    outH = h;
    return true;
}

} // namespace

// -------------------------
// Basic commands
// -------------------------

void CommandExit::execute() {
    info() << "Exiting...\n";
}

void CommandHelp::execute() {
    std::cout
        << "Commands:\n"
        << "  help\n"
        << "  exit\n"
        << "  create slideshow <name...>\n"
        << "  open <file.pptx>\n"
        << "  save <file.pptx>\n"
        << "  autosave on|off\n"
        << "  nextfile / prevfile\n"
        << "  add slide [name...]\n"
        << "  remove <index>\n"
        << "  move <fromIndex> <toIndex>\n"
        << "  goto <index>\n"
        << "  next / prev\n"
        << "  show\n"
        << "  preview\n"
        << "  undo / redo\n"
        << "\nShape commands:\n"
        << "  list shapes\n"
        << "  add rect <x> <y> <w> <h> [text...]\n"
        << "  add ellipse <x> <y> <w> <h> [text...]\n"
        << "  add text <x> <y> [text...]\n"
        << "  add image <x> <y> <path> [w h]\n"
        << "  remove shape <idx>\n"
        << "  move shape <idx> <x> <y>\n"
        << "  resize shape <idx> <w> <h>\n"
        << "  text shape <idx> [text...]\n"
        << "  duplicate shape <idx> [dx dy]\n";
}

CommandCreateSlideshow::CommandCreateSlideshow(Controller& c, const std::vector<std::string>& name)
    : ctrl(c), nameTokens(name) {}

void CommandCreateSlideshow::execute() {
    std::string name;
    for (size_t i = 0; i < nameTokens.size(); ++i) {
        if (i) name += " ";
        name += nameTokens[i];
    }
    if (name.empty()) name = "Presentation";

    ctrl.getSlideshows().clear();
    ctrl.getPresentationOrder().clear();
    ctrl.getPresentationIndex().clear();

    ctrl.getSlideshows().push_back(SlideShow(name));
    ctrl.getPresentationOrder().push_back(name);
    ctrl.getCurrentIndex() = 0;
    ctrl.rebuildUiIndex();

    success() << "Created slideshow: " << name << "\n";
}

CommandOpen::CommandOpen(Controller& c, const std::vector<std::string>& a)
    : ctrl(c), args(a) {}

void CommandOpen::execute() {
    if (args.empty()) {
        error() << "Usage: open <file.pptx>\n";
        return;
    }
    const std::string file = args[0];

    std::vector<SlideShow> slideshows;
    std::map<std::string, size_t> index;
    std::vector<std::string> order;
    size_t currentIndex = 0;

    if (!PPTXSerializer::load(slideshows, index, order, currentIndex, file)) {
        error() << "Failed to open PPTX: " << file << "\n";
        return;
    }

    ctrl.getSlideshows() = std::move(slideshows);
    ctrl.getPresentationIndex() = std::move(index);
    ctrl.getPresentationOrder() = std::move(order);

    if (ctrl.getSlideshows().empty()) {
        ctrl.getCurrentIndex() = 0;
    } else if (currentIndex >= ctrl.getSlideshows().size()) {
        ctrl.getCurrentIndex() = 0;
    } else {
        ctrl.getCurrentIndex() = currentIndex;
    }

    ctrl.rebuildUiIndex();
    success() << "Opened: " << file << "\n";
}

CommandSave::CommandSave(const std::string& f) : file(f) {}

void CommandSave::execute() {
    auto& ctrl = Controller::instance();
    if (ctrl.getSlideshows().empty()) {
        error() << "Nothing to save (no presentation loaded)\n";
        return;
    }
    if (file.empty()) {
        error() << "Usage: save <file.pptx>\n";
        return;
    }

    if (!PPTXSerializer::save(ctrl.getSlideshows(), ctrl.getPresentationOrder(), file)) {
        error() << "Failed to save: " << file << "\n";
        return;
    }
    success() << "Saved: " << file << "\n";
}

CommandAutoSave::CommandAutoSave(Controller& c, const std::vector<std::string>& a)
    : ctrl(c), args(a) {}

void CommandAutoSave::execute() {
    if (args.empty()) {
        info() << "autosave is " << (ctrl.getAutoSaveOnExit() ? "on" : "off") << "\n";
        return;
    }
    std::string v = toLower(args[0]);
    if (v == "on" || v == "1" || v == "true") ctrl.setAutoSaveOnExit(true);
    else if (v == "off" || v == "0" || v == "false") ctrl.setAutoSaveOnExit(false);
    else {
        error() << "Usage: autosave on|off\n";
        return;
    }
    success() << "autosave is now " << (ctrl.getAutoSaveOnExit() ? "on" : "off") << "\n";
}

CommandNextFile::CommandNextFile(Controller& c) : ctrl(c) {}
void CommandNextFile::execute() {
    if (ctrl.getSlideshows().empty()) {
        error() << "No presentation loaded.\n";
        return;
    }
    size_t& idx = ctrl.getCurrentIndex();
    const size_t n = ctrl.getSlideshows().size();
    idx = (idx + 1) % n;
    ctrl.rebuildUiIndex();
    info() << "Switched to presentation " << (idx + 1) << " / " << n << "\n";
}

CommandPrevFile::CommandPrevFile(Controller& c) : ctrl(c) {}
void CommandPrevFile::execute() {
    if (ctrl.getSlideshows().empty()) {
        error() << "No presentation loaded.\n";
        return;
    }
    size_t& idx = ctrl.getCurrentIndex();
    const size_t n = ctrl.getSlideshows().size();
    idx = (idx + n - 1) % n;
    ctrl.rebuildUiIndex();
    info() << "Switched to presentation " << (idx + 1) << " / " << n << "\n";
}

CommandAddSlide::CommandAddSlide(Controller& c, const std::vector<std::string>& a)
    : ctrl(c), args(a) {}

void CommandAddSlide::execute() {
    if (ctrl.getSlideshows().empty()) {
        error() << "No presentation loaded.\n";
        return;
    }
    SlideShow& ss = ctrl.getCurrentSlideshow();

    Slide slide;
    ss.getSlides().push_back(slide);
    ss.setCurrentIndex(ss.getSlides().size() - 1);

    success() << "Added slide. Total: " << ss.getSlides().size() << "\n";
}

CommandRemoveSlide::CommandRemoveSlide(Controller& c, const std::vector<std::string>& a)
    : ctrl(c), args(a) {}

void CommandRemoveSlide::execute() {
    if (ctrl.getSlideshows().empty()) {
        error() << "No presentation loaded.\n";
        return;
    }
    SlideShow& ss = ctrl.getCurrentSlideshow();
    if (ss.getSlides().empty()) {
        error() << "No slides.\n";
        return;
    }

    if (args.empty()) {
        error() << "Usage: remove <slideIndex>\n";
        return;
    }

    int idx = 0;
    if (!parseInt(args[0], idx) || idx < 1 || (size_t)idx > ss.getSlides().size()) {
        error() << "Invalid slide index.\n";
        return;
    }

    ss.getSlides().erase(ss.getSlides().begin() + (idx - 1));

    if (ss.getSlides().empty()) ss.setCurrentIndex(0);
    else if (ss.getCurrentIndex() >= ss.getSlides().size())
        ss.setCurrentIndex(ss.getSlides().size() - 1);

    success() << "Removed slide " << idx << "\n";
}

CommandMoveSlide::CommandMoveSlide(Controller& c, const std::vector<std::string>& a)
    : ctrl(c), args(a) {}

void CommandMoveSlide::execute() {
    if (ctrl.getSlideshows().empty()) {
        error() << "No presentation loaded.\n";
        return;
    }
    SlideShow& ss = ctrl.getCurrentSlideshow();
    if (ss.getSlides().empty()) {
        error() << "No slides.\n";
        return;
    }
    if (args.size() < 2) {
        error() << "Usage: move <fromIndex> <toIndex>\n";
        return;
    }

    int from = 0, to = 0;
    if (!parseInt(args[0], from) || !parseInt(args[1], to)) {
        error() << "Invalid indices.\n";
        return;
    }
    if (from < 1 || to < 1 || (size_t)from > ss.getSlides().size() || (size_t)to > ss.getSlides().size()) {
        error() << "Index out of range.\n";
        return;
    }

    if (from == to) return;

    Slide tmp = ss.getSlides()[(size_t)from - 1];
    ss.getSlides().erase(ss.getSlides().begin() + (from - 1));
    ss.getSlides().insert(ss.getSlides().begin() + (to - 1), tmp);

    success() << "Moved slide " << from << " -> " << to << "\n";
}

CommandGotoSlide::CommandGotoSlide(Controller& c, const std::vector<std::string>& a)
    : ctrl(c), args(a) {}

void CommandGotoSlide::execute() {
    if (ctrl.getSlideshows().empty()) {
        error() << "No presentation loaded.\n";
        return;
    }
    SlideShow& ss = ctrl.getCurrentSlideshow();
    if (ss.getSlides().empty()) {
        error() << "No slides.\n";
        return;
    }
    if (args.empty()) {
        error() << "Usage: goto <slideIndex>\n";
        return;
    }

    int idx = 0;
    if (!parseInt(args[0], idx) || idx < 1 || (size_t)idx > ss.getSlides().size()) {
        error() << "Invalid slide index.\n";
        return;
    }
    ss.setCurrentIndex((size_t)idx - 1);
    info() << "Current slide: " << idx << "\n";
}

CommandNext::CommandNext(SlideShow& s) : ss(s) {}
void CommandNext::execute() {
    if (ss.getSlides().empty()) return;
    size_t i = ss.getCurrentIndex();
    if (i + 1 < ss.getSlides().size()) ss.setCurrentIndex(i + 1);
}

CommandPrev::CommandPrev(SlideShow& s) : ss(s) {}
void CommandPrev::execute() {
    if (ss.getSlides().empty()) return;
    size_t i = ss.getCurrentIndex();
    if (i > 0) ss.setCurrentIndex(i - 1);
}

CommandShow::CommandShow(SlideShow& s) : ss(s) {}
void CommandShow::execute() {
    std::cout << "Slides: " << ss.getSlides().size()
              << ", current=" << (ss.getCurrentIndex() + 1) << "\n";
}

CommandPreview::CommandPreview(Controller& c) : ctrl(c) {}
void CommandPreview::execute() {
    if (ctrl.getSlideshows().empty()) {
        error() << "No presentation loaded.\n";
        return;
    }
    SlideShow& ss = ctrl.getCurrentSlideshow();
    if (ss.getSlides().empty()) {
        error() << "No slides.\n";
        return;
    }

    const Slide& s = ss.getSlides()[ss.getCurrentIndex()];
    const auto& shapes = s.getShapes();

    std::cout << "Preview (text-only)\n";
    std::cout << "  Slide: " << (ss.getCurrentIndex() + 1) << " / " << ss.getSlides().size() << "\n";
    std::cout << "  Shapes: " << shapes.size() << "\n";
    for (size_t i = 0; i < shapes.size(); ++i) {
        const Shape& sh = shapes[i];
        std::cout << "    #" << i << " kind=";
        if (sh.kind() == ShapeKind::Text) std::cout << "Text";
        else if (sh.kind() == ShapeKind::Image) std::cout << "Image";
        else if (sh.kind() == ShapeKind::Rect) std::cout << "Rect";
        else std::cout << "Ellipse";
        std::cout << " x=" << sh.getX() << " y=" << sh.getY()
                  << " w=" << sh.getW() << " h=" << sh.getH();
        if (sh.kind() != ShapeKind::Image) {
            std::cout << " text=\"" << sh.getText() << "\"";
        }
        std::cout << "\n";
    }

    info() << "Tip: Use SlideShowGUI for real preview.\n";
}

CommandUndo::CommandUndo(Controller& c) : ctrl(c) {}
void CommandUndo::execute() { ctrl.undo(); }

CommandRedo::CommandRedo(Controller& c) : ctrl(c) {}
void CommandRedo::execute() { ctrl.redo(); }

// -------------------------
// Shape commands
// -------------------------

CommandListShapes::CommandListShapes(Controller& c) : ctrl(c) {}
void CommandListShapes::execute() {
    SlideShow* ss = nullptr;
    Slide* slide = nullptr;
    if (!ensureCurrentSlide(ctrl, ss, slide)) return;

    const auto& shapes = slide->getShapes();
    info() << "Shapes on slide " << (ss->getCurrentIndex() + 1) << ":\n";

    if (shapes.empty()) {
        info() << "  (no shapes)\n";
        return;
    }

    for (size_t i = 0; i < shapes.size(); ++i) {
        const Shape& sh = shapes[i];
        std::string kind =
            (sh.kind() == ShapeKind::Text) ? "Text" :
            (sh.kind() == ShapeKind::Image) ? "Image" :
            (sh.kind() == ShapeKind::Rect) ? "Rect" : "Ellipse";

        std::cout << "  [" << (i + 1) << "] " << kind
                  << "  x=" << sh.getX() << " y=" << sh.getY()
                  << "  w=" << sh.getW() << " h=" << sh.getH();

        if (sh.isImage()) std::cout << "  bytes=" << sh.getImageData().size();
        if (!sh.getText().empty()) std::cout << "  text=\"" << sh.getText() << "\"";
        std::cout << "\n";
    }
}

CommandAddShape::CommandAddShape(Controller& c, const std::vector<std::string>& a)
    : ctrl(c), args(a) {}

void CommandAddShape::execute() {
    SlideShow* ss = nullptr;
    Slide* slide = nullptr;
    if (!ensureCurrentSlide(ctrl, ss, slide)) return;

    if (args.empty()) {
        error() << "Usage: add rect|ellipse|text|image ...\n";
        return;
    }

    std::string kind = toLower(args[0]);

    // add rect x y w h [text...]
    if (kind == "rect" || kind == "rectangle") {
        if (args.size() < 5) {
            error() << "Usage: add rect <x> <y> <w> <h> [text...]\n";
            return;
        }
        int x=0,y=0,w=0,h=0;
        if (!parseInt(args[1],x) || !parseInt(args[2],y) || !parseInt(args[3],w) || !parseInt(args[4],h)) {
            error() << "Invalid numbers.\n";
            return;
        }
        if (w < 1) w = 1;
        if (h < 1) h = 1;
        std::string text = (args.size() > 5) ? joinFrom(args, 5) : "Text";

        Shape sh("Rectangle", x, y, ShapeKind::Rect, w, h);
        sh.setText(text);
        slide->addShape(std::move(sh));
        success() << "Added rectangle.\n";
        return;
    }

    // add ellipse x y w h [text...]
    if (kind == "ellipse" || kind == "oval") {
        if (args.size() < 5) {
            error() << "Usage: add ellipse <x> <y> <w> <h> [text...]\n";
            return;
        }
        int x=0,y=0,w=0,h=0;
        if (!parseInt(args[1],x) || !parseInt(args[2],y) || !parseInt(args[3],w) || !parseInt(args[4],h)) {
            error() << "Invalid numbers.\n";
            return;
        }
        if (w < 1) w = 1;
        if (h < 1) h = 1;
        std::string text = (args.size() > 5) ? joinFrom(args, 5) : "Text";

        Shape sh("Ellipse", x, y, ShapeKind::Ellipse, w, h);
        sh.setText(text);
        slide->addShape(std::move(sh));
        success() << "Added ellipse.\n";
        return;
    }

    // add text x y [text...]
    if (kind == "text") {
        if (args.size() < 3) {
            error() << "Usage: add text <x> <y> [text...]\n";
            return;
        }
        int x=0,y=0;
        if (!parseInt(args[1],x) || !parseInt(args[2],y)) {
            error() << "Invalid numbers.\n";
            return;
        }
        std::string text = (args.size() > 3) ? joinFrom(args, 3) : "Text";
        Shape sh(text, x, y);
        sh.setText(text);
        slide->addShape(std::move(sh));
        success() << "Added text.\n";
        return;
    }

    // add image x y path [w h]
    if (kind == "image" || kind == "img") {
        if (args.size() < 4) {
            error() << "Usage: add image <x> <y> <path> [w h]\n";
            return;
        }
        int x=0,y=0;
        if (!parseInt(args[1],x) || !parseInt(args[2],y)) {
            error() << "Invalid numbers.\n";
            return;
        }

        const std::string path = args[3];

        int w = 1, h = 1;
        if (args.size() >= 6) {
            int tw=0, th=0;
            if (parseInt(args[4], tw) && parseInt(args[5], th) && tw > 0 && th > 0) {
                w = tw; h = th;
            }
        }

        std::vector<uint8_t> pngBytes;
        int imgW = 0, imgH = 0;
        if (!loadAnyImageAsPngBytes(path, pngBytes, imgW, imgH)) return;

        Shape sh("Image", x, y, std::move(pngBytes));
        if (w > 1 && h > 1) { sh.setW(w); sh.setH(h); } // otherwise PPTXSerializer uses PNG size
        slide->addShape(std::move(sh));

        success() << "Added image.\n";
        return;
    }

    error() << "Unknown shape kind: " << kind << "\n";
}

CommandRemoveShape::CommandRemoveShape(Controller& c, const std::vector<std::string>& a)
    : ctrl(c), args(a) {}

void CommandRemoveShape::execute() {
    SlideShow* ss = nullptr;
    Slide* slide = nullptr;
    if (!ensureCurrentSlide(ctrl, ss, slide)) return;

    // accepts: ["shape","2"] or ["2"]
    size_t pos = 0;
    if (!args.empty() && toLower(args[0]) == "shape") pos = 1;
    if (args.size() <= pos) {
        error() << "Usage: remove shape <idx>\n";
        return;
    }

    int idx = 0;
    if (!parseInt(args[pos], idx)) { error() << "Invalid idx.\n"; return; }

    auto& shapes = slide->getShapes();
    if (idx < 1 || (size_t)idx > shapes.size()) { error() << "Index out of range.\n"; return; }

    shapes.erase(shapes.begin() + (idx - 1));
    success() << "Removed shape " << idx << "\n";
}

CommandMoveShape::CommandMoveShape(Controller& c, const std::vector<std::string>& a)
    : ctrl(c), args(a) {}

void CommandMoveShape::execute() {
    SlideShow* ss = nullptr;
    Slide* slide = nullptr;
    if (!ensureCurrentSlide(ctrl, ss, slide)) return;

    // move shape <idx> <x> <y>
    size_t pos = 0;
    if (!args.empty() && toLower(args[0]) == "shape") pos = 1;
    if (args.size() < pos + 3) {
        error() << "Usage: move shape <idx> <x> <y>\n";
        return;
    }

    int idx=0,x=0,y=0;
    if (!parseInt(args[pos], idx) || !parseInt(args[pos+1], x) || !parseInt(args[pos+2], y)) {
        error() << "Invalid numbers.\n";
        return;
    }

    auto& shapes = slide->getShapes();
    if (idx < 1 || (size_t)idx > shapes.size()) { error() << "Index out of range.\n"; return; }

    Shape& sh = shapes[(size_t)idx - 1];
    sh.setX(x);
    sh.setY(y);
    success() << "Moved shape " << idx << " to (" << x << "," << y << ")\n";
}

CommandResizeShape::CommandResizeShape(Controller& c, const std::vector<std::string>& a)
    : ctrl(c), args(a) {}

void CommandResizeShape::execute() {
    SlideShow* ss = nullptr;
    Slide* slide = nullptr;
    if (!ensureCurrentSlide(ctrl, ss, slide)) return;

    // resize shape <idx> <w> <h>
    size_t pos = 0;
    if (!args.empty() && toLower(args[0]) == "shape") pos = 1;
    if (args.size() < pos + 3) {
        error() << "Usage: resize shape <idx> <w> <h>\n";
        return;
    }

    int idx=0,w=0,h=0;
    if (!parseInt(args[pos], idx) || !parseInt(args[pos+1], w) || !parseInt(args[pos+2], h)) {
        error() << "Invalid numbers.\n";
        return;
    }
    if (w < 1) w = 1;
    if (h < 1) h = 1;

    auto& shapes = slide->getShapes();
    if (idx < 1 || (size_t)idx > shapes.size()) { error() << "Index out of range.\n"; return; }

    Shape& sh = shapes[(size_t)idx - 1];
    sh.setW(w);
    sh.setH(h);
    success() << "Resized shape " << idx << " to (" << w << "x" << h << ")\n";
}

CommandSetShapeText::CommandSetShapeText(Controller& c, const std::vector<std::string>& a)
    : ctrl(c), args(a) {}

void CommandSetShapeText::execute() {
    SlideShow* ss = nullptr;
    Slide* slide = nullptr;
    if (!ensureCurrentSlide(ctrl, ss, slide)) return;

    // text shape <idx> [text...]
    size_t pos = 0;
    if (!args.empty() && toLower(args[0]) == "shape") pos = 1;
    if (args.size() < pos + 1) {
        error() << "Usage: text shape <idx> [text...]\n";
        return;
    }

    int idx=0;
    if (!parseInt(args[pos], idx)) { error() << "Invalid idx.\n"; return; }

    auto& shapes = slide->getShapes();
    if (idx < 1 || (size_t)idx > shapes.size()) { error() << "Index out of range.\n"; return; }

    std::string text = (args.size() > pos + 1) ? joinFrom(args, pos + 1) : "";
    shapes[(size_t)idx - 1].setText(text);
    success() << "Set text for shape " << idx << "\n";
}

CommandDuplicateShape::CommandDuplicateShape(Controller& c, const std::vector<std::string>& a)
    : ctrl(c), args(a) {}

void CommandDuplicateShape::execute() {
    SlideShow* ss = nullptr;
    Slide* slide = nullptr;
    if (!ensureCurrentSlide(ctrl, ss, slide)) return;

    // duplicate shape <idx> [dx dy]
    size_t pos = 0;
    if (!args.empty() && toLower(args[0]) == "shape") pos = 1;
    if (args.size() < pos + 1) {
        error() << "Usage: duplicate shape <idx> [dx dy]\n";
        return;
    }

    int idx=0;
    if (!parseInt(args[pos], idx)) { error() << "Invalid idx.\n"; return; }

    int dx = 20, dy = 20;
    if (args.size() >= pos + 3) {
        int tdx=0, tdy=0;
        if (parseInt(args[pos+1], tdx) && parseInt(args[pos+2], tdy)) {
            dx = tdx; dy = tdy;
        }
    }

    auto& shapes = slide->getShapes();
    if (idx < 1 || (size_t)idx > shapes.size()) { error() << "Index out of range.\n"; return; }

    Shape copy = shapes[(size_t)idx - 1];
    copy.setX(copy.getX() + dx);
    copy.setY(copy.getY() + dy);
    slide->addShape(std::move(copy));

    success() << "Duplicated shape " << idx << "\n";
}
