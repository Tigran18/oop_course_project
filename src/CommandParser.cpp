#include "../include/CommandParser.hpp"
#include "../include/Tokenizer.hpp"
#include "../include/Commands.hpp"
#include "../include/Controller.hpp"
#include <algorithm>
#include <cctype>

std::unique_ptr<ICommand> CommandParser::parse(std::istream& in) {
    std::string line;
    if (!std::getline(in, line)) {
        return std::make_unique<CommandEmpty>();
    }
    auto tokens = Tokenizer::tokenizeCommandLine(line);
    if (tokens.empty()) {
        return std::make_unique<CommandEmpty>();
    }
    const Token& cmdTok = tokens[0];
    if (!cmdTok.isCommand()) {
        return std::make_unique<CommandSafety>("[ERR] First token must be a command name");
    }
    std::string cmd = cmdTok.value;
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
    std::vector<std::string> args;
    for (size_t i = 1; i < tokens.size(); ++i) {
        const Token& t = tokens[i];
        args.push_back(t.isQuoted() ? t.unquoted() : t.value);
    }
    if (cmd == "next" || cmd == "prev" || cmd == "show") {
        if (Controller::instance().getSlideshows().empty()) {
            return std::make_unique<CommandSafety>("[ERR] No presentations loaded.");
        }
        SlideShow& ss = Controller::instance().getCurrentSlideshow();
        if (cmd == "next") {
            return std::make_unique<CommandNext>(ss);
        }
        if (cmd == "prev") {
            return std::make_unique<CommandPrev>(ss);
        }
        return std::make_unique<CommandShow>(ss);
    }
    if (cmd == "nextfile" || cmd == "prevfile") {
        if (Controller::instance().getSlideshows().empty()) {
            return std::make_unique<CommandSafety>("[ERR] No presentations to navigate.");
        }
        if (cmd == "nextfile") {
            return std::make_unique<CommandNextFile>(Controller::instance().getCurrentIndex(),
                                                     Controller::instance().getSlideshows());
        }
        return std::make_unique<CommandPrevFile>(Controller::instance().getCurrentIndex(),
                                                 Controller::instance().getSlideshows());
    }
    if (cmd == "goto") {
        if (Controller::instance().getSlideshows().empty()) {
            return std::make_unique<CommandSafety>("[ERR] No presentations for 'goto'.");
        }
        return std::make_unique<CommandGoto>(
            Controller::instance().getCurrentIndex(),
            Controller::instance().getSlideshows(),
            Controller::instance().getPresentationIndex(),
            Controller::instance().getPresentationOrder(),
            args);
    }
    if (cmd == "help") {
        return std::make_unique<CommandHelp>();
    }
    if (cmd == "exit") {
        return std::make_unique<CommandExit>();
    }
    if (cmd == "save") {
        return std::make_unique<CommandSave>(Controller::instance(), args);
    }
    if (cmd == "load") {
        return std::make_unique<CommandLoad>(Controller::instance(), args);
    }
    if (cmd == "add") {
        return std::make_unique<CommandAdd>(Controller::instance(), args);
    }
    return std::make_unique<CommandSafety>("[ERR] Unknown command: " + cmd);
}