#include "gui/MainWindow.hpp"

#include "gui/SlideList.hpp"
#include "gui/CanvasView.hpp"
#include "gui/QtLogStream.hpp"
#include "gui/SettingsDialog.hpp"

#include <QRegularExpression>
#include <QApplication>
#include <QLineEdit>
#include <QTextEdit>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QMenuBar>
#include <QToolBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QCloseEvent>
#include <QSettings>

#include <sstream>
#include <iostream>

// Core
#include "Controller.hpp"
#include "CommandParser.hpp"
#include "ICommand.hpp"
#include "Commands.hpp"
#include "PPTXSerializer.hpp"
#include "SlideShow.hpp"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupUi();
    setupMenusAndToolbars();

    // ✅ Redirect stdout+stderr to GUI log
    logStream_ = new QtLogStream(this);
    oldCout_ = std::cout.rdbuf(logStream_);
    oldCerr_ = std::cerr.rdbuf(logStream_);

    connect(logStream_, &QtLogStream::textReady, this, [this](const QString& line) {
        log_->append(line);
    });

    loadSettings();
    syncUiFromModel();

    std::cout << "GUI started" << "\n";
}

MainWindow::~MainWindow()
{
    saveSettings();

    if (oldCout_) std::cout.rdbuf(oldCout_);
    if (oldCerr_) std::cerr.rdbuf(oldCerr_);
}

static QString stripAnsi(QString s)
{
    // Removes typical ANSI CSI sequences like ESC[32m ESC[0m etc.
    static const QRegularExpression re(QStringLiteral("\x1B\\[[0-9;]*[A-Za-z]"));
    s.remove(re);
    return s;
}


void MainWindow::setupUi()
{
    setWindowTitle("SlideShow GUI");

    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);

    // Top splitter: slide list | canvas
    auto* topSplitter = new QSplitter(Qt::Horizontal);

    slideList_ = new SlideList;
    canvas_ = new CanvasView;

    topSplitter->addWidget(slideList_);
    topSplitter->addWidget(canvas_);
    topSplitter->setStretchFactor(0, 1);
    topSplitter->setStretchFactor(1, 4);

    // Command input + log
    commandInput_ = new QLineEdit;
    commandInput_->setPlaceholderText(
        "Enter command... (same as CLI: help, create slideshow X, add slide ..., next, save file.pptx)");

    log_ = new QTextEdit;
    log_->setReadOnly(true);

    root->addWidget(topSplitter, 4);
    root->addWidget(commandInput_);
    root->addWidget(log_, 2);

    setCentralWidget(central);

    connect(commandInput_, &QLineEdit::returnPressed,
            this, &MainWindow::onCommandEntered);

    connect(slideList_, &SlideList::slideChosen,
            this, &MainWindow::onSlideChosen);

    statusBar()->showMessage("Ready");
}

void MainWindow::setupMenusAndToolbars()
{
    // Menus
    auto* fileMenu = menuBar()->addMenu("File");
    auto* viewMenu = menuBar()->addMenu("View");
    auto* toolsMenu = menuBar()->addMenu("Tools");
    auto* helpMenu = menuBar()->addMenu("Help");

    // File
    QAction* actNew = fileMenu->addAction("New Presentation");
    QAction* actOpen = fileMenu->addAction("Open...");
    QAction* actSave = fileMenu->addAction("Save As...");
    fileMenu->addSeparator();
    QAction* actExit = fileMenu->addAction("Exit");

    connect(actNew, &QAction::triggered, this, &MainWindow::newPresentation);
    connect(actOpen, &QAction::triggered, this, &MainWindow::openPptx);
    connect(actSave, &QAction::triggered, this, &MainWindow::saveAsPptx);
    connect(actExit, &QAction::triggered, this, [this]() { close(); });

    // View
    darkModeAction_ = viewMenu->addAction("Dark Mode");
    darkModeAction_->setCheckable(true);
    connect(darkModeAction_, &QAction::toggled, this, &MainWindow::toggleDarkMode);

    QAction* gridAction = viewMenu->addAction("Show Grid");
    gridAction->setCheckable(true);
    connect(gridAction, &QAction::toggled, this, [this](bool on) {
        showGrid_ = on;
        canvas_->setShowGrid(on);
        syncUiFromModel();
    });

    viewMenu->addSeparator();
    QAction* zoomIn = viewMenu->addAction("Zoom In");
    QAction* zoomOut = viewMenu->addAction("Zoom Out");
    QAction* zoomReset = viewMenu->addAction("Reset Zoom");
    connect(zoomIn, &QAction::triggered, canvas_, &CanvasView::zoomIn);
    connect(zoomOut, &QAction::triggered, canvas_, &CanvasView::zoomOut);
    connect(zoomReset, &QAction::triggered, canvas_, &CanvasView::zoomReset);

    // Tools
    QAction* settings = toolsMenu->addAction("Settings...");
    connect(settings, &QAction::triggered, this, &MainWindow::openSettings);

    // Help
    QAction* actHelp = helpMenu->addAction("Help (commands)");
    QAction* actAbout = helpMenu->addAction("About");
    connect(actHelp, &QAction::triggered, this, &MainWindow::showHelp);
    connect(actAbout, &QAction::triggered, this, &MainWindow::about);

    // Toolbar like your screenshot: New Slide | Text | Draw | Image
    auto* tb = addToolBar("Main");
    tb->setMovable(false);

    QAction* tbNewSlide = tb->addAction("New Slide");
    QAction* tbText = tb->addAction("Text");
    QAction* tbDraw = tb->addAction("Draw");
    QAction* tbImage = tb->addAction("Image");

    connect(tbNewSlide, &QAction::triggered, this, &MainWindow::addNewSlide);
    connect(tbText, &QAction::triggered, this, &MainWindow::addTextShape);
    connect(tbDraw, &QAction::triggered, this, &MainWindow::addDrawStub);
    connect(tbImage, &QAction::triggered, this, &MainWindow::addImageShape);
}

QString MainWindow::quoteIfNeeded(const QString& s) const
{
    if (s.contains(' ') || s.contains('\t'))
        return "\"" + s + "\"";
    return s;
}

void MainWindow::loadSettings()
{
    QSettings st;
    darkMode_ = st.value("ui/darkMode", false).toBool();
    showGrid_ = st.value("ui/showGrid", false).toBool();

    if (darkModeAction_) darkModeAction_->setChecked(darkMode_);
    canvas_->setShowGrid(showGrid_);

    // apply now
    toggleDarkMode(darkMode_);
}

void MainWindow::saveSettings()
{
    QSettings st;
    st.setValue("ui/darkMode", darkMode_);
    st.setValue("ui/showGrid", showGrid_);
}

void MainWindow::toggleDarkMode(bool enabled)
{
    darkMode_ = enabled;

    if (enabled) {
        qApp->setStyleSheet(R"(
            QWidget { background-color: #2b2b2b; color: #dddddd; }
            QLineEdit, QTextEdit, QListWidget {
                background-color: #1e1e1e;
                color: #dddddd;
                border: 1px solid #444444;
            }
            QMenuBar, QMenu, QToolBar {
                background-color: #2b2b2b;
                color: #dddddd;
            }
        )");
        statusBar()->showMessage("Dark mode enabled");
    } else {
        qApp->setStyleSheet("");
        statusBar()->showMessage("Dark mode disabled");
    }
}

void MainWindow::openSettings()
{
    SettingsDialog dlg(this);

    dlg.setDarkMode(darkMode_);
    dlg.setShowGrid(showGrid_);

    // Use core autosave flag
    auto& ctrl = Controller::instance();
    dlg.setAutosaveOnExit(ctrl.getAutoSaveOnExit());

    if (dlg.exec() != QDialog::Accepted)
        return;

    // Apply
    if (darkModeAction_) darkModeAction_->setChecked(dlg.darkMode());
    showGrid_ = dlg.showGrid();
    canvas_->setShowGrid(showGrid_);
    ctrl.setAutoSaveOnExit(dlg.autosaveOnExit());

    syncUiFromModel();
    statusBar()->showMessage("Settings applied");
}

void MainWindow::showHelp()
{
    executeCommand("help");
}

void MainWindow::about()
{
    QMessageBox::information(this, "About",
                             "SlideShow GUI\n"
                             "Command-based Qt frontend over your core slideshow engine.");
}

void MainWindow::newPresentation()
{
    // Create a default presentation name
    executeCommand("create slideshow Presentation");
}

void MainWindow::openPptx()
{
    const QString path = QFileDialog::getOpenFileName(
        this, "Open PPTX", QString(), "PowerPoint (*.pptx)");

    if (path.isEmpty()) return;

    executeCommand("open " + quoteIfNeeded(path));
}

void MainWindow::saveAsPptx()
{
    const QString path = QFileDialog::getSaveFileName(
        this, "Save PPTX As", "export.pptx", "PowerPoint (*.pptx)");

    if (path.isEmpty()) return;

    executeCommand("save " + quoteIfNeeded(path));
}

void MainWindow::addNewSlide()
{
    auto& ctrl = Controller::instance();
    if (ctrl.getSlideshows().empty()) {
        executeCommand("create slideshow Presentation");
    }
    executeCommand("add slide Slide 100 100");
}

void MainWindow::addTextShape()
{
    auto& ctrl = Controller::instance();
    if (ctrl.getSlideshows().empty()) {
        executeCommand("create slideshow Presentation");
        executeCommand("add slide Slide 100 100");
    }
    executeCommand("add slide Text 200 200");
}

void MainWindow::addDrawStub()
{
    // Your core currently doesn’t store vector drawing shapes,
    // so this creates a normal “Draw” text shape placeholder.
    auto& ctrl = Controller::instance();
    if (ctrl.getSlideshows().empty()) {
        executeCommand("create slideshow Presentation");
        executeCommand("add slide Slide 100 100");
    }
    executeCommand("add slide Draw 300 300");
}

void MainWindow::addImageShape()
{
    auto& ctrl = Controller::instance();
    if (ctrl.getSlideshows().empty()) {
        executeCommand("create slideshow Presentation");
        executeCommand("add slide Slide 100 100");
    }

    const QString path = QFileDialog::getOpenFileName(
        this, "Add Image", QString(), "Images (*.png *.jpg *.jpeg *.bmp)");

    if (path.isEmpty()) return;

    // CLI syntax from your help:
    // add slide image <name> <x> <y> "<path>"
    executeCommand(QString("add slide image Image 100 100 %1").arg(quoteIfNeeded(path)));
}

void MainWindow::onCommandEntered()
{
    const QString cmd = commandInput_->text().trimmed();
    if (cmd.isEmpty()) return;

    executeCommand(cmd, true);
    commandInput_->clear();
}

void MainWindow::onSlideChosen(int index)
{
    // Slide indices in CLI are typically 1-based
    executeCommand(QString("goto %1").arg(index + 1));
}

bool MainWindow::executeCommand(const QString& cmd, bool echoToLog)
{
    if (echoToLog) {
        log_->append("> " + cmd);
    }

    // parse using core CommandParser (istream-based)
    CommandParser parser;
    std::istringstream in(cmd.toStdString());

    std::unique_ptr<ICommand> icmd = parser.parse(in);
    if (!icmd) {
        std::cout << "[ERR] Invalid command\n";
        return false;
    }

    auto& ctrl = Controller::instance();

    // mimic CLI snapshot behavior for undo/redo
    bool shouldSnapshot =
        dynamic_cast<CommandCreateSlideshow*>(icmd.get()) ||
        dynamic_cast<CommandAddSlide*>(icmd.get()) ||
        dynamic_cast<CommandRemoveSlide*>(icmd.get()) ||
        dynamic_cast<CommandMoveSlide*>(icmd.get()) ||
        dynamic_cast<CommandGotoSlide*>(icmd.get()) ||
        dynamic_cast<CommandNext*>(icmd.get()) ||
        dynamic_cast<CommandPrev*>(icmd.get()) ||
        dynamic_cast<CommandNextFile*>(icmd.get()) ||
        dynamic_cast<CommandPrevFile*>(icmd.get());

    if (shouldSnapshot) {
        ctrl.snapshot();
    }

    icmd->execute();

    // keep indices consistent for UI
    ctrl.rebuildUiIndex();

    syncUiFromModel();
    return true;
}

void MainWindow::syncUiFromModel()
{
    auto& ctrl = Controller::instance();

    if (ctrl.getSlideshows().empty()) {
        slideList_->showNoPresentation();
        canvas_->showMessage("No presentation.\nUse toolbar -> New Slide\nor command: create slideshow Name");
        return;
    }

    SlideShow& ss = ctrl.getCurrentSlideshow();
    const auto& slides = ss.getSlides();

    const int count = static_cast<int>(slides.size());
    const int cur = count > 0 ? static_cast<int>(ss.getCurrentIndex()) : 0;

    slideList_->setSlideCount(count, cur);

    if (slides.empty()) {
        canvas_->showMessage("Empty presentation.\nUse: add slide Name X Y");
        return;
    }

    if (cur >= 0 && cur < count) {
        canvas_->renderSlide(slides[cur]);
    } else {
        canvas_->showMessage("Current slide index out of range");
    }
}

void MainWindow::closeEvent(QCloseEvent* e)
{
    auto& ctrl = Controller::instance();

    // autosave on exit (same idea as CLI)
    if (ctrl.getAutoSaveOnExit() && !ctrl.getSlideshows().empty()) {
        const std::string file = "AutoExport.pptx";
        std::cout << "Autosaving to " << file << "\n";
        PPTXSerializer::save(ctrl.getSlideshows(), ctrl.getPresentationOrder(), file);
    }

    QMainWindow::closeEvent(e);
}
