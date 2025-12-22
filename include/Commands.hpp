#pragma once
#include "ICommand.hpp"
#include "Controller.hpp"

#include <string>
#include <vector>
#include <memory>

// -------------------------
// Basic commands
// -------------------------
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
    std::vector<std::string> nameTokens;
public:
    CommandCreateSlideshow(Controller& c, const std::vector<std::string>& name);
    void execute() override;
};

class CommandOpen : public ICommand {
    Controller& ctrl;
    std::vector<std::string> args;
public:
    CommandOpen(Controller& c, const std::vector<std::string>& a);
    void execute() override;
};

class CommandSave : public ICommand {
    std::string file;
public:
    CommandSave(const std::string& f);
    void execute() override;
};

class CommandAutoSave : public ICommand {
    Controller& ctrl;
    std::vector<std::string> args;
public:
    CommandAutoSave(Controller& c, const std::vector<std::string>& a);
    void execute() override;
};

class CommandNextFile : public ICommand {
    Controller& ctrl;
public:
    CommandNextFile(Controller& c);
    void execute() override;
};

class CommandPrevFile : public ICommand {
    Controller& ctrl;
public:
    CommandPrevFile(Controller& c);
    void execute() override;
};

class CommandAddSlide : public ICommand {
    Controller& ctrl;
    std::vector<std::string> args;
public:
    CommandAddSlide(Controller& c, const std::vector<std::string>& a);
    void execute() override;
};

class CommandRemoveSlide : public ICommand {
    Controller& ctrl;
    std::vector<std::string> args;
public:
    CommandRemoveSlide(Controller& c, const std::vector<std::string>& a);
    void execute() override;
};

class CommandMoveSlide : public ICommand {
    Controller& ctrl;
    std::vector<std::string> args;
public:
    CommandMoveSlide(Controller& c, const std::vector<std::string>& a);
    void execute() override;
};

class CommandGotoSlide : public ICommand {
    Controller& ctrl;
    std::vector<std::string> args;
public:
    CommandGotoSlide(Controller& c, const std::vector<std::string>& a);
    void execute() override;
};

class CommandNext : public ICommand {
    class SlideShow& ss;
public:
    CommandNext(class SlideShow& s);
    void execute() override;
};

class CommandPrev : public ICommand {
    class SlideShow& ss;
public:
    CommandPrev(class SlideShow& s);
    void execute() override;
};

class CommandShow : public ICommand {
    class SlideShow& ss;
public:
    CommandShow(class SlideShow& s);
    void execute() override;
};

class CommandPreview : public ICommand {
    Controller& ctrl;
public:
    CommandPreview(Controller& c);
    void execute() override;
};

class CommandUndo : public ICommand {
    Controller& ctrl;
public:
    CommandUndo(Controller& c);
    void execute() override;
};

class CommandRedo : public ICommand {
    Controller& ctrl;
public:
    CommandRedo(Controller& c);
    void execute() override;
};

// -------------------------
// Shape commands (core)
// -------------------------
// These fix your linker errors (vtable/typeinfo) because execute() is defined in Commands.cpp

class CommandListShapes : public ICommand {
    Controller& ctrl;
public:
    CommandListShapes(Controller& c);
    void execute() override;
};

class CommandAddShape : public ICommand {
    Controller& ctrl;
    std::vector<std::string> args;
public:
    CommandAddShape(Controller& c, const std::vector<std::string>& a);
    void execute() override;
};

class CommandRemoveShape : public ICommand {
    Controller& ctrl;
    std::vector<std::string> args;
public:
    CommandRemoveShape(Controller& c, const std::vector<std::string>& a);
    void execute() override;
};

class CommandMoveShape : public ICommand {
    Controller& ctrl;
    std::vector<std::string> args;
public:
    CommandMoveShape(Controller& c, const std::vector<std::string>& a);
    void execute() override;
};

class CommandResizeShape : public ICommand {
    Controller& ctrl;
    std::vector<std::string> args;
public:
    CommandResizeShape(Controller& c, const std::vector<std::string>& a);
    void execute() override;
};

class CommandSetShapeText : public ICommand {
    Controller& ctrl;
    std::vector<std::string> args;
public:
    CommandSetShapeText(Controller& c, const std::vector<std::string>& a);
    void execute() override;
};

class CommandDuplicateShape : public ICommand {
    Controller& ctrl;
    std::vector<std::string> args;
public:
    CommandDuplicateShape(Controller& c, const std::vector<std::string>& a);
    void execute() override;
};
