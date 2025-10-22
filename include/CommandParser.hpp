#pragma once

#include <memory>
#include <map>
#include <string>
#include <vector>
#include <istream>

#include "Functions.hpp"
#include "Tokenizer.hpp"
#include "ICommand.hpp"
#include "SlideShow.hpp"

class CommandParser {
public:
    static std::unique_ptr<ICommand> parse(std::istream& in,
        SlideShow& currentSlideShow,
        size_t& currentIndex,
        std::vector<SlideShow>& slideshows,
        std::map<std::string, size_t>& presentationIndex,
        std::vector<std::string>& presentationOrder,
        bool& exitProgram);
};
