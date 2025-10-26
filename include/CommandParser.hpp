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

extern std::vector<SlideShow> slideshows;
extern std::map<std::string, size_t> presentationIndex;
extern std::vector<std::string> presentationOrder;
extern size_t currentIndex;

class CommandParser {
public:
    static std::unique_ptr<ICommand> parse(std::istream& in
    );
};
