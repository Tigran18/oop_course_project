#include "gui/SlideList.hpp"
#include <QSignalBlocker>

SlideList::SlideList(QWidget* parent)
    : QListWidget(parent)
{
    connect(this, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row >= 0) emit slideChosen(row);
    });
}

void SlideList::showNoPresentation()
{
    QSignalBlocker blocker(this);
    clear();
    addItem("<<no presentation>>");
    setCurrentRow(0);
}

void SlideList::setSlideCount(int count, int current)
{
    QSignalBlocker blocker(this);

    clear();
    for (int i = 0; i < count; ++i) {
        addItem(QString("Slide %1").arg(i + 1));
    }
    if (count > 0) {
        setCurrentRow(std::max(0, std::min(current, count - 1)));
    }
}
