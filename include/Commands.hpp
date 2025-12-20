#pragma once
#include "ICommand.hpp"
#include "Controller.hpp"
#include "SlideShow.hpp"
#include "Shape.hpp"
#include "Functions.hpp"
#include "PPTXSerializer.hpp"
#include "Color.hpp"
#include "lodepng.h"

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>

class CommandExit : public ICommand { 
public:
    void execute() override; 
};

class CommandHelp : public ICommand { 
public:
    void execute() override; 
};

class CommandCreateSlideshow : public ICommand {
    Controller& ctrl;
    std::vector<std::string> args;
public:
    CommandCreateSlideshow(Controller& c, std::vector<std::string> a) : ctrl(c), args(std::move(a)) {}
    void execute() override;
};

class CommandOpen : public ICommand {
    Controller& ctrl;
    std::vector<std::string> args;
public:
    CommandOpen(Controller& c, std::vector<std::string> a)
        : ctrl(c), args(std::move(a)) {}
    void execute() override;
};

class CommandSave : public ICommand {
    std::string filename;
public:
    CommandSave(const std::string& f) : filename(f) {}
    void execute() override;
};


class CommandAutoSave : public ICommand {
    Controller& ctrl;
    std::vector<std::string> args;
public:
    CommandAutoSave(Controller& c, std::vector<std::string> a)
        : ctrl(c), args(std::move(a)) {}
    void execute() override;
};


class CommandAddSlide : public ICommand {
    Controller& ctrl;
    std::vector<std::string> args;
public:
    CommandAddSlide(Controller& c, std::vector<std::string> a) : ctrl(c), args(std::move(a)) {}
    void execute() override;
};

class CommandRemoveSlide : public ICommand {
    Controller& ctrl;
    std::vector<std::string> args;
public:
    CommandRemoveSlide(Controller& c, std::vector<std::string> a) : ctrl(c), args(std::move(a)) {}
    void execute() override;
};

class CommandMoveSlide : public ICommand {
    Controller& ctrl;
    std::vector<std::string> args;
public:
    CommandMoveSlide(Controller& c, std::vector<std::string> a) : ctrl(c), args(std::move(a)) {}
    void execute() override;
};

class CommandGotoSlide : public ICommand {
    Controller& ctrl;
    std::vector<std::string> args;
public:
    CommandGotoSlide(Controller& c, std::vector<std::string> a) : ctrl(c), args(std::move(a)) {}
    void execute() override;
};

class CommandNext : public ICommand {
    SlideShow& ss;
public:
    CommandNext(SlideShow& s) : ss(s) {}
    void execute() override;
};

class CommandShow : public ICommand {
    SlideShow& ss;
public:
    explicit CommandShow(SlideShow& ss_) : ss(ss_) {}
    void execute() override;
};

class CommandPrev : public ICommand {
    SlideShow& ss;
public:
    CommandPrev(SlideShow& s) : ss(s) {}
    void execute() override;
};

class CommandPreview : public ICommand {
    Controller& ctrl;
public:
    CommandPreview(Controller& c) : ctrl(c) {}
    void execute() override;
};


class CommandNextFile : public ICommand {
    Controller& ctrl;
public:
    CommandNextFile(Controller& c) : ctrl(c) {}
    void execute() override;
};

class CommandPrevFile : public ICommand {
    Controller& ctrl;
public:
    CommandPrevFile(Controller& c) : ctrl(c) {}
    void execute() override;
};

class CommandUndo : public ICommand {
    Controller& ctrl;
public:
    CommandUndo(Controller& c) : ctrl(c) {}
    void execute() override;
};

class CommandRedo : public ICommand {
    Controller& ctrl;
public:
    CommandRedo(Controller& c) : ctrl(c) {}
    void execute() override;
};