#include "../include/CommandParser.hpp"
#include "../include/Commands.hpp"
#include "../include/Tokenizer.hpp"
#include "../include/Controller.hpp"
#include "../include/Functions.hpp"
#include <iostream>
#include <algorithm>

std::unique_ptr<ICommand> CommandParser::parse(std::istream& in) {
    std::string line;
    if (!std::getline(in, line)) {
        return std::unique_ptr<ICommand>(new CommandExit());
    }
    if (line.empty()) {
        return nullptr;
    }
    auto tokens = Tokenizer::tokenizeCommandLine(line);
    if (tokens.empty()) {
        return nullptr;
    }
    const Token& cmdTok = tokens[0];
    if (!cmdTok.isCommand()) {
        std::cout << "[ERR] First token must be command\n";
        return nullptr;
    }
    std::string cmd = cmdTok.asString();
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
    std::vector<std::string> args;
    for (size_t i = 1; i < tokens.size(); ++i) {
        const Token& t = tokens[i];
        if (t.isQuoted()) {
            args.push_back(t.unquoted());
        }
        else if (t.type == TokenType::NUMBER) {
            args.push_back(std::to_string((int)t.asNumber()));
        }
        else {
            args.push_back(t.asString());
        }
    }
    auto& ctrl = Controller::instance();
    if (cmd == "exit") {
        return std::unique_ptr<ICommand>(new CommandExit());
    }
    if (cmd == "help") {
        return std::unique_ptr<ICommand>(new CommandHelp());
    }
    if (cmd == "create" && !args.empty() && args[0] == "slideshow") {
        std::vector<std::string> name(args.begin() + 1, args.end());
        return std::unique_ptr<ICommand>(new CommandCreateSlideshow(ctrl, name));
    }
    if (cmd == "open") {
        return std::unique_ptr<ICommand>(new CommandOpen(ctrl, args));
    }
    if (cmd == "save") {
        return std::unique_ptr<ICommand>(new CommandSave(ctrl, args));
    }
    if (cmd == "autosave") {
        return std::unique_ptr<ICommand>(new CommandAutoSave(ctrl, args));
    }
    if (cmd == "nextfile") {
        return std::unique_ptr<ICommand>(new CommandNextFile(ctrl));
    }
    if (cmd == "prevfile") {
        return std::unique_ptr<ICommand>(new CommandPrevFile(ctrl));
    }
    if (cmd == "add" && !args.empty() && args[0] == "slide") {
        if (ctrl.getSlideshows().empty()) {
            std::cout << "[ERR] No presentation loaded. Use 'create slideshow <name>' or 'open <file.pptx>' first.\n";
            return nullptr;
        }
        std::vector<std::string> slideArgs(args.begin() + 1, args.end());
        return std::unique_ptr<ICommand>(new CommandAddSlide(ctrl, slideArgs));
    }
    if (ctrl.getSlideshows().empty() && 
        (cmd == "remove" || cmd == "move" || cmd == "goto" || 
         cmd == "next" || cmd == "prev" || cmd == "show")) {
        std::cout << "[ERR] No presentation loaded.\n";
        return nullptr;
    }
    if (cmd == "remove") {
        return std::unique_ptr<ICommand>(new CommandRemoveSlide(ctrl, args));
    }
    if (cmd == "move") {
        return std::unique_ptr<ICommand>(new CommandMoveSlide(ctrl, args));
    }
    if (cmd == "goto") {
        return std::unique_ptr<ICommand>(new CommandGotoSlide(ctrl, args));
    }
    if (cmd == "next") {
        return std::unique_ptr<ICommand>(new CommandNext(ctrl.getCurrentSlideshow()));
    }
    if (cmd == "prev") {
        return std::unique_ptr<ICommand>(new CommandPrev(ctrl.getCurrentSlideshow()));
    }
    if (cmd == "show") {
        return std::unique_ptr<ICommand>(new CommandShow(ctrl.getCurrentSlideshow()));
    }
    if (cmd == "undo") {
        return std::unique_ptr<ICommand>(new CommandUndo(ctrl));
    }
    if (cmd == "redo") {
        return std::unique_ptr<ICommand>(new CommandRedo(ctrl));
    }
    std::cout << "[ERR] Unknown command: " << cmd << "\n";
    return nullptr;
}
