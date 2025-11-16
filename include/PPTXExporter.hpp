#pragma once
#include <string>
#include <vector>

class SlideShow;

class PPTXExporter {
public:
    static bool exportToRealPPTX(const std::vector<SlideShow>& slideshows,
                                 const std::vector<std::string>& order,
                                 const std::string& outputPath);
};