#pragma once

#include <QMainWindow>
#include <memory>
#include <streambuf>

class QLineEdit;
class QTextEdit;
class QAction;

class SlideList;
class CanvasView;
class QtLogStream;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* e) override;

private slots:
    void onCommandEntered();
    void onSlideChosen(int index);

    void newPresentation();
    void openPptx();
    void saveAsPptx();

    void addNewSlide();
    void addTextShape();
    void addDrawStub();
    void addImageShape();

    void toggleDarkMode(bool enabled);
    void openSettings();

    void showHelp();
    void about();

private:
    void setupUi();
    void setupMenusAndToolbars();

    void loadSettings();
    void saveSettings();

    bool executeCommand(const QString& cmd, bool echoToLog = true);
    void syncUiFromModel();

    QString quoteIfNeeded(const QString& s) const;

private:
    SlideList* slideList_ = nullptr;
    CanvasView* canvas_ = nullptr;
    QLineEdit* commandInput_ = nullptr;
    QTextEdit* log_ = nullptr;

    QtLogStream* logStream_ = nullptr;
    std::streambuf* oldCout_ = nullptr;
    std::streambuf* oldCerr_ = nullptr;

    QAction* darkModeAction_ = nullptr;

    bool darkMode_ = false;
    bool showGrid_ = false;
};
