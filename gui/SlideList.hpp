#pragma once

#include <QListWidget>

class SlideList : public QListWidget {
    Q_OBJECT
public:
    explicit SlideList(QWidget* parent = nullptr);

    void showNoPresentation();
    void setSlideCount(int count, int currentIndex);

signals:
    void slideChosen(int index); // 0-based

private slots:
    void onSelectionChanged();

private:
    bool suppress_ = false;
};
