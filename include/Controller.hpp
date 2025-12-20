#pragma once

#include "SlideShow.hpp"

#include <vector>
#include <string>
#include <map>

class Controller
{
public:
    static Controller& instance();

    std::vector<SlideShow>& getSlideshows();
    const std::vector<SlideShow>& getSlideshows() const;

    std::vector<std::string>& getPresentationOrder();
    const std::vector<std::string>& getPresentationOrder() const;

    std::map<std::string, size_t>& getPresentationIndex();
    const std::map<std::string, size_t>& getPresentationIndex() const;

    size_t& getCurrentIndex();
    size_t getCurrentIndex() const;

    SlideShow& getCurrentSlideshow();
    const SlideShow& getCurrentSlideshow() const;

    void rebuildUiIndex();

    void setAutoSaveOnExit(bool on);
    bool getAutoSaveOnExit() const;

    void snapshot();
    bool undo();
    bool redo();

    void run();

private:
    Controller() = default;

    struct SnapshotState
    {
        std::vector<SlideShow> slideshows;
        std::vector<std::string> order;
        std::map<std::string, size_t> index;
        size_t currentIndex = 0;
        bool autosaveOnExit = false;
    };

    SnapshotState packState() const;
    void restoreState(const SnapshotState& st);

    void normalizeCurrentIndex();
    void ensureOrderIndexConsistent();

private:
    std::vector<SlideShow> slideshows_;
    std::vector<std::string> presentationOrder_;
    std::map<std::string, size_t> presentationIndex_;
    size_t currentIndex_ = 0;
    bool autosaveOnExit_ = false;

    std::vector<SnapshotState> undo_;
    std::vector<SnapshotState> redo_;
};
