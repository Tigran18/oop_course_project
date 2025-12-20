#include "gui/MainWindow.hpp"

#include "gui/SlideList.hpp"
#include "gui/CanvasView.hpp"
#include "gui/QtLogStream.hpp"
#include "gui/SettingsDialog.hpp"

#include <QApplication>
#include <QLineEdit>
#include <QTextEdit>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>
#include <QMenuBar>
#include <QToolBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QCloseEvent>
#include <QSettings>
#include <QDockWidget>
#include <QFormLayout>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QPalette>

#include <sstream>
#include <iostream>

#include "Controller.hpp"
#include "CommandParser.hpp"
#include "ICommand.hpp"
#include "Commands.hpp"
#include "PPTXSerializer.hpp"
#include "Color.hpp"
#include "SlideShow.hpp"
#include "Slide.hpp"
#include "Shape.hpp"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupUi();
    setupMenusAndToolbars();
    setupPropertiesDock();

    logStream_ = new QtLogStream(this);
    oldCout_ = std::cout.rdbuf(logStream_);
    oldCerr_ = std::cerr.rdbuf(logStream_);

    connect(canvas_, &CanvasView::shapeMoved, this, &MainWindow::onShapeMoved);


    connect(logStream_, &QtLogStream::textReady,
        this,
        [this](const QString& line) { log_->append(line); },
        Qt::QueuedConnection);


    connect(canvas_, &CanvasView::shapeSelected, this, &MainWindow::onShapeSelected);
    connect(canvas_, &CanvasView::selectionCleared, this, &MainWindow::onSelectionCleared);

    loadSettings();
    syncUiFromModel();

    std::cout << "GUI started\n";
}

MainWindow::~MainWindow()
{
    saveSettings();

    if (oldCout_) std::cout.rdbuf(oldCout_);
    if (oldCerr_) std::cerr.rdbuf(oldCerr_);
}

void MainWindow::onShapeMoved(int idx, int x, int y)
{
    auto& ctrl = Controller::instance();
    if (ctrl.getSlideshows().empty()) return;

    SlideShow& ss = ctrl.getCurrentSlideshow();
    if (ss.getSlides().empty()) return;

    auto& slide = ss.getSlides()[ss.getCurrentIndex()];
    auto& shapes = slide.getShapes();

    if (idx < 0 || (size_t)idx >= shapes.size()) return;

    Shape& sh = shapes[(size_t)idx];

    // Avoid spamming snapshots if nothing really changed
    if (sh.getX() == x && sh.getY() == y) return;

    ctrl.snapshot(); // so Undo/Redo works

    sh.setX(x);
    sh.setY(y);

    // Keep Properties dock in sync if this is the selected shape
    if (selectedShape_ == idx) {
        xSpin_->setValue(x);
        ySpin_->setValue(y);
    }
}


void MainWindow::setupUi()
{
    setWindowTitle("SlideShow GUI");

    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);

    auto* topSplitter = new QSplitter(Qt::Horizontal);

    slideList_ = new SlideList;
    canvas_ = new CanvasView;

    topSplitter->addWidget(slideList_);
    topSplitter->addWidget(canvas_);
    topSplitter->setStretchFactor(0, 1);
    topSplitter->setStretchFactor(1, 4);

    commandInput_ = new QLineEdit;
    commandInput_->setPlaceholderText("Enter command... (help, open file.pptx, save out.pptx, goto 1, next, prev)");

    log_ = new QTextEdit;
    log_->setReadOnly(true);

    root->addWidget(topSplitter, 4);
    root->addWidget(commandInput_);
    root->addWidget(log_, 2);

    setCentralWidget(central);

    connect(commandInput_, &QLineEdit::returnPressed, this, &MainWindow::onCommandEntered);
    connect(slideList_, &SlideList::slideChosen, this, &MainWindow::onSlideChosen);

    statusBar()->showMessage("Ready");
}

void MainWindow::setupMenusAndToolbars()
{
    auto* fileMenu = menuBar()->addMenu("File");
    auto* viewMenu = menuBar()->addMenu("View");
    auto* toolsMenu = menuBar()->addMenu("Tools");
    auto* helpMenu = menuBar()->addMenu("Help");

    QAction* actNew = fileMenu->addAction("New Presentation");
    QAction* actOpen = fileMenu->addAction("Open...");
    QAction* actSave = fileMenu->addAction("Save As...");
    fileMenu->addSeparator();
    QAction* actExit = fileMenu->addAction("Exit");

    connect(actNew, &QAction::triggered, this, &MainWindow::newPresentation);
    connect(actOpen, &QAction::triggered, this, &MainWindow::openPptx);
    connect(actSave, &QAction::triggered, this, &MainWindow::saveAsPptx);
    connect(actExit, &QAction::triggered, this, [this]() { close(); });

    darkModeAction_ = viewMenu->addAction("Dark Mode");
    darkModeAction_->setCheckable(true);
    connect(darkModeAction_, &QAction::toggled, this, &MainWindow::toggleDarkMode);

    QAction* gridAction = viewMenu->addAction("Show Grid");
    gridAction->setCheckable(true);
    connect(gridAction, &QAction::toggled, this, [this](bool on) {
        showGrid_ = on;
        canvas_->setShowGrid(on);
    });

    viewMenu->addSeparator();
    QAction* zoomIn = viewMenu->addAction("Zoom In");
    QAction* zoomOut = viewMenu->addAction("Zoom Out");
    QAction* zoomReset = viewMenu->addAction("Reset Zoom");
    connect(zoomIn, &QAction::triggered, canvas_, &CanvasView::zoomIn);
    connect(zoomOut, &QAction::triggered, canvas_, &CanvasView::zoomOut);
    connect(zoomReset, &QAction::triggered, canvas_, &CanvasView::zoomReset);

    QAction* settings = toolsMenu->addAction("Settings...");
    connect(settings, &QAction::triggered, this, &MainWindow::openSettings);

    QAction* actHelp = helpMenu->addAction("Help (commands)");
    QAction* actAbout = helpMenu->addAction("About");
    connect(actHelp, &QAction::triggered, this, &MainWindow::showHelp);
    connect(actAbout, &QAction::triggered, this, &MainWindow::about);

    auto* tb = addToolBar("Main");
    tb->setMovable(false);

    QAction* tbNewSlide = tb->addAction("New Slide");
    QAction* tbText = tb->addAction("Text");
    QAction* tbRect = tb->addAction("Rectangle");
    QAction* tbEllipse = tb->addAction("Ellipse");
    QAction* tbImage = tb->addAction("Image");

    connect(tbNewSlide, &QAction::triggered, this, &MainWindow::addNewSlide);
    connect(tbText, &QAction::triggered, this, &MainWindow::addTextShape);
    connect(tbRect, &QAction::triggered, this, &MainWindow::addRectShape);
    connect(tbEllipse, &QAction::triggered, this, &MainWindow::addEllipseShape);
    connect(tbImage, &QAction::triggered, this, &MainWindow::addImageShape);
}

void MainWindow::setupPropertiesDock()
{
    propsDock_ = new QDockWidget("Properties", this);
    propsDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    auto* w = new QWidget(propsDock_);
    auto* form = new QFormLayout(w);

    selLabel_ = new QLabel("No selection");
    form->addRow("Selected:", selLabel_);

    xSpin_ = new QSpinBox;
    ySpin_ = new QSpinBox;
    wSpin_ = new QSpinBox;
    hSpin_ = new QSpinBox;

    xSpin_->setRange(0, 2000);
    ySpin_->setRange(0, 2000);
    wSpin_->setRange(1, 5000);
    hSpin_->setRange(1, 5000);

    form->addRow("X:", xSpin_);
    form->addRow("Y:", ySpin_);
    form->addRow("W:", wSpin_);
    form->addRow("H:", hSpin_);

    textEdit_ = new QLineEdit;
    form->addRow("Text:", textEdit_);

    applyBtn_ = new QPushButton("Apply");
    form->addRow(applyBtn_);

    connect(applyBtn_, &QPushButton::clicked, this, &MainWindow::applyShapeProperties);

    propsDock_->setWidget(w);
    addDockWidget(Qt::RightDockWidgetArea, propsDock_);

    onSelectionCleared();
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

    QPalette pal = qApp->palette();

    if (!enabled) {
        qApp->setPalette(QPalette());
        statusBar()->showMessage("Light mode");
        return;
    }

    pal.setColor(QPalette::Window, QColor(43, 43, 43));
    pal.setColor(QPalette::WindowText, QColor(220, 220, 220));
    pal.setColor(QPalette::Base, QColor(30, 30, 30));
    pal.setColor(QPalette::AlternateBase, QColor(43, 43, 43));
    pal.setColor(QPalette::ToolTipBase, QColor(30, 30, 30));
    pal.setColor(QPalette::ToolTipText, QColor(220, 220, 220));
    pal.setColor(QPalette::Text, QColor(220, 220, 220));
    pal.setColor(QPalette::Button, QColor(43, 43, 43));
    pal.setColor(QPalette::ButtonText, QColor(220, 220, 220));
    pal.setColor(QPalette::BrightText, Qt::red);
    pal.setColor(QPalette::Highlight, QColor(90, 140, 255));
    pal.setColor(QPalette::HighlightedText, Qt::black);

    qApp->setPalette(pal);
    statusBar()->showMessage("Dark mode");
}

void MainWindow::openSettings()
{
    SettingsDialog dlg(this);

    dlg.setDarkMode(darkMode_);
    dlg.setShowGrid(showGrid_);

    auto& ctrl = Controller::instance();
    dlg.setAutosaveOnExit(ctrl.getAutoSaveOnExit());

    if (dlg.exec() != QDialog::Accepted)
        return;

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
                             "Qt frontend for your slideshow engine.\n"
                             "Use toolbar to add shapes and the Properties dock to edit them.");
}

void MainWindow::ensurePresentation()
{
    auto& ctrl = Controller::instance();
    if (ctrl.getSlideshows().empty()) {
        executeCommand("create slideshow Presentation");
    }
}

void MainWindow::ensureSlide()
{
    ensurePresentation();
    auto& ctrl = Controller::instance();
    SlideShow& ss = ctrl.getCurrentSlideshow();
    if (ss.getSlides().empty()) {
        ctrl.snapshot();
        ss.getSlides().push_back(Slide());
        ss.setCurrentIndex(0);
        ctrl.rebuildUiIndex();
    }
}

void MainWindow::newPresentation()
{
    executeCommand("create slideshow Presentation");
}

void MainWindow::openPptx()
{
    const QString path = QFileDialog::getOpenFileName(this, "Open PPTX", QString(), "PowerPoint (*.pptx)");
    if (path.isEmpty()) return;
    executeCommand("open " + quoteIfNeeded(path));
}

void MainWindow::saveAsPptx()
{
    const QString path = QFileDialog::getSaveFileName(this, "Save PPTX As", "export.pptx", "PowerPoint (*.pptx)");
    if (path.isEmpty()) return;
    executeCommand("save " + quoteIfNeeded(path));
}

void MainWindow::addNewSlide()
{
    ensurePresentation();
    auto& ctrl = Controller::instance();
    SlideShow& ss = ctrl.getCurrentSlideshow();

    ctrl.snapshot();
    ss.getSlides().push_back(Slide());
    ss.setCurrentIndex(ss.getSlides().empty() ? 0 : ss.getSlides().size() - 1);
    ctrl.rebuildUiIndex();
    info() << "Added new slide." << '\n';
    syncUiFromModel();
}

void MainWindow::addTextShape()
{
    ensureSlide();
    auto& ctrl = Controller::instance();
    SlideShow& ss = ctrl.getCurrentSlideshow();

    ctrl.snapshot();
    ss.getSlides()[ss.getCurrentIndex()].addShape(Shape("Text", 120, 120));
    ctrl.rebuildUiIndex();
    info() << "Added text shape." << '\n';
    syncUiFromModel();
}

void MainWindow::addRectShape()
{
    ensureSlide();
    auto& ctrl = Controller::instance();
    SlideShow& ss = ctrl.getCurrentSlideshow();

    ctrl.snapshot();
    Shape sh("Rectangle", 150, 150, ShapeKind::Rect, 260, 120);
    sh.setText("Text");
    ss.getSlides()[ss.getCurrentIndex()].addShape(sh);
    ctrl.rebuildUiIndex();
    info() << "Added rectangle shape." << '\n';
    syncUiFromModel();
}

void MainWindow::addEllipseShape()
{
    ensureSlide();
    auto& ctrl = Controller::instance();
    SlideShow& ss = ctrl.getCurrentSlideshow();

    ctrl.snapshot();
    Shape sh("Ellipse", 180, 180, ShapeKind::Ellipse, 240, 140);
    sh.setText("Text");
    ss.getSlides()[ss.getCurrentIndex()].addShape(sh);
    ctrl.rebuildUiIndex();
    info() << "Added ellipse shape." << '\n';
    syncUiFromModel();
}

void MainWindow::addImageShape()
{
    ensureSlide();

    const QString path = QFileDialog::getOpenFileName(this, "Add Image", QString(), "Images (*.png *.jpg *.jpeg *.bmp)");
    if (path.isEmpty()) return;

    std::ifstream file(path.toStdString(), std::ios::binary);
    if (!file) return;

    std::vector<uint8_t> imgData((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    auto& ctrl = Controller::instance();
    SlideShow& ss = ctrl.getCurrentSlideshow();

    ctrl.snapshot();
    ss.getSlides()[ss.getCurrentIndex()].addShape(Shape("Image", 100, 100, std::move(imgData)));
    ctrl.rebuildUiIndex();
    info() << "Added image shape." << '\n';
    syncUiFromModel();
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
    executeCommand(QString("goto %1").arg(index + 1));
}

bool MainWindow::executeCommand(const QString& cmd, bool echoToLog)
{
    if (echoToLog) log_->append("> " + cmd);

    CommandParser parser;
    std::istringstream in(cmd.toStdString());

    std::unique_ptr<ICommand> icmd = parser.parse(in);
    if (!icmd) {
        std::cout << "[ERR] Invalid command\n";
        return false;
    }

    auto& ctrl = Controller::instance();

    bool shouldSnapshot =
        dynamic_cast<CommandCreateSlideshow*>(icmd.get()) ||
        dynamic_cast<CommandOpen*>(icmd.get()) ||
        dynamic_cast<CommandAddSlide*>(icmd.get()) ||
        dynamic_cast<CommandRemoveSlide*>(icmd.get()) ||
        dynamic_cast<CommandMoveSlide*>(icmd.get()) ||
        dynamic_cast<CommandGotoSlide*>(icmd.get()) ||
        dynamic_cast<CommandNext*>(icmd.get()) ||
        dynamic_cast<CommandPrev*>(icmd.get()) ||
        dynamic_cast<CommandNextFile*>(icmd.get()) ||
        dynamic_cast<CommandPrevFile*>(icmd.get());

    if (shouldSnapshot) ctrl.snapshot();

    icmd->execute();

    ctrl.rebuildUiIndex();
    syncUiFromModel();
    return true;
}

void MainWindow::syncUiFromModel()
{
    auto& ctrl = Controller::instance();

    if (ctrl.getSlideshows().empty()) {
        slideList_->showNoPresentation();
        canvas_->showMessage("No presentation.\nUse File -> New Presentation");
        return;
    }

    SlideShow& ss = ctrl.getCurrentSlideshow();
    const auto& slides = ss.getSlides();

    const int count = static_cast<int>(slides.size());
    const int cur = (count > 0) ? static_cast<int>(ss.getCurrentIndex()) : 0;

    slideList_->setSlideCount(count, cur);

    if (slides.empty()) {
        canvas_->showMessage("Empty presentation.\nUse toolbar: New Slide");
        return;
    }

    if (cur >= 0 && cur < count) {
        canvas_->renderSlide(slides[(size_t)cur]);
    } else {
        canvas_->showMessage("Current slide index out of range");
    }
}

void MainWindow::onShapeSelected(int idx)
{
    selectedShape_ = idx;

    auto& ctrl = Controller::instance();
    if (ctrl.getSlideshows().empty()) return;

    SlideShow& ss = ctrl.getCurrentSlideshow();
    if (ss.getSlides().empty()) return;

    auto& slide = ss.getSlides()[ss.getCurrentIndex()];
    auto& shapes = slide.getShapes();
    if (idx < 0 || (size_t)idx >= shapes.size()) return;

    const Shape& sh = shapes[(size_t)idx];

    selLabel_->setText(QString("#%1 (%2)").arg(idx).arg(
        sh.kind() == ShapeKind::Text ? "Text" :
        sh.kind() == ShapeKind::Image ? "Image" :
        sh.kind() == ShapeKind::Rect ? "Rect" : "Ellipse"
    ));

    xSpin_->setValue(sh.getX());
    ySpin_->setValue(sh.getY());
    wSpin_->setValue(sh.getW() > 0 ? sh.getW() : 1);
    hSpin_->setValue(sh.getH() > 0 ? sh.getH() : 1);
    textEdit_->setText(QString::fromStdString(sh.getText()));
}

void MainWindow::onSelectionCleared()
{
    selectedShape_ = -1;
    selLabel_->setText("No selection");
    xSpin_->setValue(0);
    ySpin_->setValue(0);
    wSpin_->setValue(1);
    hSpin_->setValue(1);
    textEdit_->setText("");
}

void MainWindow::applyShapeProperties()
{
    if (selectedShape_ < 0) return;

    auto& ctrl = Controller::instance();
    if (ctrl.getSlideshows().empty()) return;

    SlideShow& ss = ctrl.getCurrentSlideshow();
    if (ss.getSlides().empty()) return;

    auto& slide = ss.getSlides()[ss.getCurrentIndex()];
    auto& shapes = slide.getShapes();
    if ((size_t)selectedShape_ >= shapes.size()) return;

    ctrl.snapshot();

    Shape& sh = shapes[(size_t)selectedShape_];
    sh.setX(xSpin_->value());
    sh.setY(ySpin_->value());
    sh.setW(wSpin_->value());
    sh.setH(hSpin_->value());
    sh.setText(textEdit_->text().toStdString());

    ctrl.rebuildUiIndex();
    syncUiFromModel();
}

void MainWindow::closeEvent(QCloseEvent* e)
{
    auto& ctrl = Controller::instance();

    if (ctrl.getAutoSaveOnExit() && !ctrl.getSlideshows().empty()) {
        std::string file = "AutoExport.pptx";
        if (std::filesystem::exists(file)) {
            char buf[32] = {};
            std::time_t t = std::time(nullptr);
            std::tm* tm = std::localtime(&t);
            if (tm) {
                std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", tm);
                file = std::string("AutoExport_") + buf + ".pptx";
            } else {
                file = "AutoExport_autosave.pptx";
            }
        }

        std::cout << "Autosaving to " << file << "\n";
        PPTXSerializer::save(ctrl.getSlideshows(), ctrl.getPresentationOrder(), file);
    }

    QMainWindow::closeEvent(e);
}
