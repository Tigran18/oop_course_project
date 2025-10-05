#include "../include/CommandParser.hpp"
#include <algorithm>

const std::set<std::string> CommandParser::validCommands = {
    "next", "prev", "show", "nextfile", "prevfile", "exit", "help", "goto", "listfiles"
};

Command CommandParser::parse(std::istream& in) {
    std::string line;
    std::getline(in, line);
    if (in.eof()) {
        return Command("", {}, false);
    }
    auto tokens = Tokenizer::tokenizeCommandLine(line);
    if (tokens.empty()) {
        return Command("", {}, false);
    }
    std::string command = tokens[0];
    std::transform(command.begin(), command.end(), command.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    if (validCommands.find(command) == validCommands.end()) {
        return Command(command, {}, false);
    }
    std::vector<std::string> args(tokens.begin() + 1, tokens.end());
    for (auto& arg : args) {
        arg = utils::trim(arg);
    }
    if (command == "goto" && (args.empty() || args.size() > 2)) {
        return Command(command, args, false);
    }
    return Command(command, args, true);
}
