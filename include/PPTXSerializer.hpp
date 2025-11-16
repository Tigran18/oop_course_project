#pragma once
#include <vector>
#include <string>
#include <map>

class SlideShow;

enum class LoadResult { 
    SUCCESS, 
    NOT_FOUND, 
    CORRUPTED, 
    IO_ERROR 
};

class PPTXSerializer {
public:
    static bool save(const std::vector<SlideShow>& slideshows,
                     const std::vector<std::string>& order,
                     const std::string& pptxPath);

    static LoadResult load(std::vector<SlideShow>& slideshows,
                           std::map<std::string, size_t>& index,
                           std::vector<std::string>& order,
                           size_t& currentIdx,
                           const std::string& pptxPath);
};