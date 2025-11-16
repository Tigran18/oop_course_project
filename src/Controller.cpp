#include "../include/Controller.hpp"
#include "../include/CommandParser.hpp"
#include "../include/Commands.hpp"
#include "../include/SlideShow.hpp"
#include "../include/Color.hpp"
#include <iostream>

Controller& Controller::instance() {
    static Controller inst;
    return inst;
}

SlideShow& Controller::getCurrentSlideshow() {
    if (slideshows.empty()) {
        static SlideShow dummy("<<NO PRESENTATION>>");
        return dummy;
    }
    return slideshows[currentIndex];
}

void Controller::pushSnapshot() {
    std::vector<SlideShow> copy;
    copy.reserve(slideshows.size());
    for (const auto& ss : slideshows)
        copy.push_back(ss);
    undoStack.push_back(std::move(copy));
    redoStack.clear();
    if (undoStack.size() > 50) undoStack.pop_front();
}

void Controller::rebuildIndexAndOrder() {
    presentationIndex.clear();
    presentationOrder.clear();
    for (size_t i = 0; i < slideshows.size(); ++i) {
        const auto& name = slideshows[i].getFilename();
        presentationIndex[name] = i;
        presentationOrder.push_back(name);
    }
    if (!presentationOrder.empty()) {
        currentIndex = presentationIndex[presentationOrder.back()];
    } else {
        currentIndex = 0;
    }
}

bool Controller::undo() {
    if (undoStack.empty()) return false;

    // Save current state to redo stack
    std::vector<SlideShow> currentCopy;
    currentCopy.reserve(slideshows.size());
    for (const auto& ss : slideshows) {
        currentCopy.push_back(ss);
    }
    redoStack.push_back(std::move(currentCopy));

    // Restore from undo stack
    slideshows = std::move(undoStack.back());
    undoStack.pop_back();

    rebuildIndexAndOrder();

    if (redoStack.size() > 50) redoStack.pop_front();

    return true;
}

bool Controller::redo() {
    if (redoStack.empty()) {
        return false;
    }
    // Save current state to undo stack
    std::vector<SlideShow> currentCopy;
    currentCopy.reserve(slideshows.size());
    for (const auto& ss : slideshows) {
        currentCopy.push_back(ss);
    }
    undoStack.push_back(std::move(currentCopy));

    // Restore from redo stack
    slideshows = std::move(redoStack.back());
    redoStack.pop_back();

    rebuildIndexAndOrder();

    if (undoStack.size() > 50) {
        undoStack.pop_front();
    }
    return true;
}

void Controller::run() {
    CommandParser parser;
    info() << "Multi-PPTX CLI Slideshow\n";
    info() << "Type 'help' for commands\n";
    while (true) {
        std::string prompt;
        if (slideshows.empty()) {
            prompt = "[No presentations] > ";
        } 
        else {
            const auto& currentSS = slideshows[currentIndex];
            size_t slideCount = currentSS.getSlides().size();
            size_t currentSlide = slideCount > 0 ? currentSS.getCurrentIndex() + 1 : 0;
            prompt = "[pp " + std::to_string(currentIndex + 1) + "/" + std::to_string(slideshows.size()) +
                     " | slide " + (slideCount > 0 ? std::to_string(currentSlide) : "0") + "/" + std::to_string(slideCount) + "] > ";
        }
        std::cout << BLUE << prompt << RESET;
        auto cmd = parser.parse(std::cin);
        if (!cmd) {
            continue;
        }
        // Determine if command modifies state
        bool shouldSnapshot = (
            dynamic_cast<CommandCreateSlideshow*>(cmd.get()) ||
            dynamic_cast<CommandAddSlide*>(cmd.get()) ||
            dynamic_cast<CommandRemoveSlide*>(cmd.get()) ||
            dynamic_cast<CommandMoveSlide*>(cmd.get()) ||
            dynamic_cast<CommandGotoSlide*>(cmd.get()) ||
            dynamic_cast<CommandNext*>(cmd.get()) ||
            dynamic_cast<CommandPrev*>(cmd.get()) ||
            dynamic_cast<CommandNextFile*>(cmd.get()) ||
            dynamic_cast<CommandPrevFile*>(cmd.get())
        );

        // Take snapshot BEFORE executing the command
        if (shouldSnapshot) {
            pushSnapshot();
        }
        cmd->execute();
        if (dynamic_cast<CommandExit*>(cmd.get())) {
            break;
        }
    }
}

std::vector<SlideShow>& Controller::getSlideshows() { 
    return slideshows; 
}
    
const std::vector<SlideShow>& Controller::getSlideshows() const { 
    return slideshows; 
}

std::map<std::string, size_t>& Controller::getPresentationIndex() { 
    return presentationIndex; 
}

std::vector<std::string>& Controller::getPresentationOrder() { 
    return presentationOrder; 
}

size_t& Controller::getCurrentIndex() { 
    return currentIndex; 
}
 
bool Controller::getAutoSaveOnExit() const { 
    return autoSaveOnExit; 
}

void Controller::setAutoSaveOnExit(bool b) { 
    autoSaveOnExit = b; 
}