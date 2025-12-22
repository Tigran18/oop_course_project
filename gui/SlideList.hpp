#pragma once

#include <QListWidget>

class SlideList : public QListWidget {
    Q_OBJECT
public:
    explicit SlideList(QWidget* parent = nullptr);

    void showNoPresentation();
    void setSlideCount(int count, int current);

signals:
    void slideChosen(int index);
};
