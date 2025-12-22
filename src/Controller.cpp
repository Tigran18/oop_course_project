#include "Controller.hpp"

#include "CommandParser.hpp"
#include "ICommand.hpp"
#include "Commands.hpp"
#include "Color.hpp"

#include "PPTXSerializer.hpp"
#include "Functions.hpp"

#include <iostream>
#include <sstream>
#include <memory>
#include <algorithm>

Controller& Controller::instance()
{
    static Controller inst;
    return inst;
}

std::vector<SlideShow>& Controller::getSlideshows() { return slideshows_; }
const std::vector<SlideShow>& Controller::getSlideshows() const { return slideshows_; }

std::vector<std::string>& Controller::getPresentationOrder() { return presentationOrder_; }
const std::vector<std::string>& Controller::getPresentationOrder() const { return presentationOrder_; }

std::map<std::string, size_t>& Controller::getPresentationIndex() { return presentationIndex_; }
const std::map<std::string, size_t>& Controller::getPresentationIndex() const { return presentationIndex_; }

size_t& Controller::getCurrentIndex() { return currentIndex_; }
size_t Controller::getCurrentIndex() const { return currentIndex_; }

void Controller::normalizeCurrentIndex()
{
    if (slideshows_.empty()) {
        currentIndex_ = 0;
        return;
    }
    if (currentIndex_ >= slideshows_.size()) currentIndex_ = 0;
}

void Controller::ensureOrderIndexConsistent()
{
    presentationIndex_.clear();

    if (presentationOrder_.empty()) {
        presentationOrder_.reserve(slideshows_.size());
        for (const auto& ss : slideshows_) {
            presentationOrder_.push_back(ss.getFilename());
        }
    }

    std::vector<std::string> newOrder;
    newOrder.reserve(presentationOrder_.size());

    for (const auto& name : presentationOrder_) {
        auto it = std::find_if(slideshows_.begin(), slideshows_.end(),
                               [&](const SlideShow& s) { return s.getFilename() == name; });
        if (it != slideshows_.end()) newOrder.push_back(name);
    }

    for (const auto& ss : slideshows_) {
        const std::string& name = ss.getFilename();
        if (std::find(newOrder.begin(), newOrder.end(), name) == newOrder.end())
            newOrder.push_back(name);
    }

    presentationOrder_ = std::move(newOrder);

    for (size_t i = 0; i < slideshows_.size(); ++i) {
        presentationIndex_[slideshows_[i].getFilename()] = i;
    }

    normalizeCurrentIndex();
}

void Controller::rebuildUiIndex()
{
    ensureOrderIndexConsistent();
}

SlideShow& Controller::getCurrentSlideshow()
{
    normalizeCurrentIndex();
    if (slideshows_.empty()) {
        slideshows_.emplace_back("Presentation");
        presentationOrder_.clear();
        presentationIndex_.clear();
        ensureOrderIndexConsistent();
        currentIndex_ = 0;
    }
    normalizeCurrentIndex();
    return slideshows_[currentIndex_];
}

const SlideShow& Controller::getCurrentSlideshow() const
{
    if (slideshows_.empty()) {
        static SlideShow dummy("Presentation");
        return dummy;
    }
    size_t idx = currentIndex_;
    if (idx >= slideshows_.size()) idx = 0;
    return slideshows_[idx];
}

void Controller::setAutoSaveOnExit(bool on) { autosaveOnExit_ = on; }
bool Controller::getAutoSaveOnExit() const { return autosaveOnExit_; }

Controller::SnapshotState Controller::packState() const
{
    SnapshotState st;
    st.slideshows = slideshows_;
    st.order = presentationOrder_;
    st.index = presentationIndex_;
    st.currentIndex = currentIndex_;
    st.autosaveOnExit = autosaveOnExit_;
    return st;
}

void Controller::restoreState(const SnapshotState& st)
{
    slideshows_ = st.slideshows;
    presentationOrder_ = st.order;
    presentationIndex_ = st.index;
    currentIndex_ = st.currentIndex;
    autosaveOnExit_ = st.autosaveOnExit;
    ensureOrderIndexConsistent();
}

void Controller::snapshot()
{
    undo_.push_back(packState());
    redo_.clear();
}

bool Controller::undo()
{
    if (undo_.empty()) return false;

    redo_.push_back(packState());
    SnapshotState st = undo_.back();
    undo_.pop_back();
    restoreState(st);
    return true;
}

bool Controller::redo()
{
    if (redo_.empty()) return false;

    undo_.push_back(packState());
    SnapshotState st = redo_.back();
    redo_.pop_back();
    restoreState(st);
    return true;
}

void Controller::run()
{
    std::cout << "SlideShowCLI started\n";
    std::string line;

    while (true) {
        std::string prompt = "> ";
        std::cout << BLUE << prompt << RESET;
        if (!std::getline(std::cin, line)) break;

        std::string trimmed = line;
        while (!trimmed.empty() && (trimmed.back() == '\r' || trimmed.back() == '\n'))
            trimmed.pop_back();
        if (trimmed.empty()) continue;

        if (trimmed == "exit") break;

        std::istringstream iss(trimmed);
        auto cmd = CommandParser::parse(iss);

        if (!cmd) {
            error() << "Invalid command\n";
            continue;
        }

        bool shouldSnapshot =
            dynamic_cast<CommandCreateSlideshow*>(cmd.get()) ||
            dynamic_cast<CommandOpen*>(cmd.get()) ||
            dynamic_cast<CommandAddSlide*>(cmd.get()) ||
            dynamic_cast<CommandRemoveSlide*>(cmd.get()) ||
            dynamic_cast<CommandMoveSlide*>(cmd.get()) ||
            dynamic_cast<CommandAddShape*>(cmd.get()) ||
            dynamic_cast<CommandRemoveShape*>(cmd.get()) ||
            dynamic_cast<CommandMoveShape*>(cmd.get()) ||
            dynamic_cast<CommandResizeShape*>(cmd.get()) ||
            dynamic_cast<CommandSetShapeText*>(cmd.get()) ||
            dynamic_cast<CommandDuplicateShape*>(cmd.get());

        if (shouldSnapshot) snapshot();

        cmd->execute();
        rebuildUiIndex();
    }

    // Autosave on exit (CLI)
    if (getAutoSaveOnExit() && !slideshows_.empty()) {
        std::string desired = slideshows_[currentIndex_].getFilename();
        if (desired.empty()) desired = "AutoExport.pptx";
        const std::string outFile = utils::makeUniquePptxPath(desired);
        if (PPTXSerializer::save(slideshows_, presentationOrder_, outFile)) {
            info() << "Autosaved to: " << outFile << "\n";
        } else {
            error() << "Autosave failed: " << outFile << "\n";
        }
    }
}
