#pragma once

#include <string>
#include <vector>
#include <map>

class SlideShow;

class PPTXSerializer {
public:

    // Save multiple slideshows merged into one PPTX.
    // slideshows        : all slideshow objects
    // order             : presentation names in the desired output order
    // outputPath        : final .pptx path
    //
    // Returns true on success.
    static bool save(const std::vector<SlideShow>& slideshows,
                     const std::vector<std::string>& order,
                     const std::string& outputPath);

    // Optional: load is unimplemented (your original file returns false)
    static bool load(std::vector<SlideShow>& slideshows,
                     std::map<std::string, size_t>& presentationIndex,
                     std::vector<std::string>& presentationOrder,
                     size_t& currentIndex,
                     const std::string& path);
};

