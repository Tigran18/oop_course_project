#pragma once
#include "SlideShow.hpp"
#include <string>
#include <vector>
#include <map>

class PPTXSerializer {
public:
    // Save slideshows to a valid PPTX using template.pptx
    static bool save(const std::vector<SlideShow>& slideshows,
                     const std::vector<std::string>& order,
                     const std::string& outputFile);

    // Load slideshows from an existing PPTX (optional for now)
    static bool load(std::vector<SlideShow>& slideshows,
                     std::map<std::string, size_t>& presentationIndex,
                     std::vector<std::string>& presentationOrder,
                     size_t& currentIndex,
                     const std::string& inputFile);

};