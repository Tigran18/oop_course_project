#include "../include/CommandParser.hpp"
#include <algorithm>

#include "CommandParser.hpp"
#include "Tokenizer.hpp"
#include "Commands.hpp"
#include <algorithm>

std::unique_ptr<ICommand> CommandParser::parse(std::istream& in,
    SlideShow& currentSlideShow,
    size_t& currentIndex,
    std::vector<SlideShow>& slideshows,
    std::map<std::string, size_t>& presentationIndex,
    std::vector<std::string>& presentationOrder,
    bool& exitProgram)
{
    std::string line;
    if (!std::getline(in, line)) return std::make_unique<EmptyCommand>();
    auto tokens = Tokenizer::tokenizeCommandLine(line);
    if (tokens.empty()) return std::make_unique<EmptyCommand>();
    std::string cmdName = tokens[0];
    std::transform(cmdName.begin(), cmdName.end(), cmdName.begin(), ::tolower);
    std::vector<std::string> args(tokens.begin() + 1, tokens.end());
    if (cmdName == "next") {
        return std::make_unique<NextCommand>(currentSlideShow);
    }
    if (cmdName == "prev") {
        return std::make_unique<PrevCommand>(currentSlideShow);
    }
    if (cmdName == "show") {
        return std::make_unique<ShowCommand>(currentSlideShow);
    }
    if (cmdName == "nextfile") {
        return std::make_unique<NextFileCommand>(currentIndex, slideshows);
    }
    if (cmdName == "prevfile") {
        return std::make_unique<PrevFileCommand>(currentIndex, slideshows);
    }
    if (cmdName == "goto") {
        return std::make_unique<GotoCommand>(currentIndex, slideshows, presentationIndex, presentationOrder, args);
    }
    if (cmdName == "help") {
        return std::make_unique<HelpCommand>();
    }
    if (cmdName == "exit") {
        return std::make_unique<ExitCommand>(exitProgram);
    }
    return std::make_unique<EmptyCommand>();
}
