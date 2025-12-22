#include "gui/SlideList.hpp"

#include <QSignalBlocker>
#include <algorithm>

SlideList::SlideList(QWidget* parent)
    : QListWidget(parent)
{
    connect(this, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row >= 0) emit slideChosen(row);
    });
}

void SlideList::showNoPresentation()
{
    // Block signals to avoid triggering MainWindow::onSlideChosen while we
    // are updating the list programmatically.
    QSignalBlocker blocker(this);
    clear();
    addItem("<<no presentation>>");
    setCurrentRow(0);
}

void SlideList::setSlideCount(int count, int current)
{
    // IMPORTANT: SlideList emits currentRowChanged when we call setCurrentRow
    // (and sometimes during clear/addItem). MainWindow listens to this signal
    // to run a "goto <n>" command.
    //
    // When MainWindow syncs the UI from the model, it calls this function.
    // If we don't block signals, we create an infinite recursion:
    //   syncUiFromModel() -> setSlideCount() -> currentRowChanged -> onSlideChosen()
    //   -> goto -> syncUiFromModel() -> ... (stack overflow / crash)
    QSignalBlocker blocker(this);
    clear();
    for (int i = 0; i < count; ++i) {
        addItem(QString("Slide %1").arg(i + 1));
    }
    if (count > 0) {
        setCurrentRow(std::max(0, std::min(current, count - 1)));
    }
}
