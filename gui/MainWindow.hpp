#pragma once

#include <QMainWindow>

class SlideList;
class CanvasView;
class QtLogStream;

class QLineEdit;
class QTextEdit;
class QAction;

class QDockWidget;
class QSpinBox;
class QLineEdit;
class QPushButton;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* e) override;

private slots:
    void onCommandEntered();
    void onSlideChosen(int index);

    void onShapeMoved(int idx, int x, int y);


    void newPresentation();
    void openPptx();
    void saveAsPptx();

    void addNewSlide();
    void addTextShape();
    void addRectShape();
    void addEllipseShape();
    void addImageShape();

    void toggleDarkMode(bool enabled);
    void openSettings();
    void showHelp();
    void about();

    void onShapeSelected(int idx);
    void onSelectionCleared();
    void applyShapeProperties();

private:
    void setupUi();
    void setupMenusAndToolbars();
    void setupPropertiesDock();

    void loadSettings();
    void saveSettings();

    void syncUiFromModel();
    bool executeCommand(const QString& cmd, bool echoToLog = false);

    QString quoteIfNeeded(const QString& s) const;

    void ensurePresentation();
    void ensureSlide();

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

    QDockWidget* propsDock_ = nullptr;
    QLabel* selLabel_ = nullptr;
    QSpinBox* xSpin_ = nullptr;
    QSpinBox* ySpin_ = nullptr;
    QSpinBox* wSpin_ = nullptr;
    QSpinBox* hSpin_ = nullptr;
    QLineEdit* textEdit_ = nullptr;
    QPushButton* applyBtn_ = nullptr;

    int selectedShape_ = -1;
};
