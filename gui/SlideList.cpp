#include "gui/SlideList.hpp"

SlideList::SlideList(QWidget* parent)
    : QListWidget(parent)
{
    connect(this, &QListWidget::currentRowChanged,
            this, &SlideList::onSelectionChanged);
}

void SlideList::showNoPresentation()
{
    suppress_ = true;
    clear();
    addItem("<<no presentation>>");
    item(0)->setFlags(item(0)->flags() & ~Qt::ItemIsSelectable);
    setCurrentRow(0);
    suppress_ = false;
}

void SlideList::setSlideCount(int count, int currentIndex)
{
    suppress_ = true;
    clear();

    for (int i = 0; i < count; ++i) {
        addItem(QString("Slide %1").arg(i + 1));
    }

    if (count > 0) {
        if (currentIndex < 0) currentIndex = 0;
        if (currentIndex >= count) currentIndex = count - 1;
        setCurrentRow(currentIndex);
    } else {
        addItem("<<empty>>");
        item(0)->setFlags(item(0)->flags() & ~Qt::ItemIsSelectable);
        setCurrentRow(0);
    }

    suppress_ = false;
}

void SlideList::onSelectionChanged()
{
    if (suppress_) return;
    int idx = currentRow();
    if (idx >= 0)
        emit slideChosen(idx);
}
