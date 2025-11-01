#pragma once
#include "SlideShow.hpp"
#include <vector>
#include <map>
#include <string>

namespace Serializer {
    bool save(const std::vector<SlideShow>& slideshows,
              const std::vector<std::string>& order,
              const std::string& filepath);

    bool load(std::vector<SlideShow>& slideshows,
              std::map<std::string, size_t>& index,
              std::vector<std::string>& order,
              size_t& currentIndex,
              const std::string& filepath);
}