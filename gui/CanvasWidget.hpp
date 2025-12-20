#pragma once
#include <QWidget>
#include <QImage>
#include <QColor>

class CanvasWidget : public QWidget {
    Q_OBJECT
public:
    enum class Tool { Draw, Text, Image };

    explicit CanvasWidget(QWidget* parent = nullptr);

    void setTool(Tool t);
    void setPenColor(const QColor& c);
    void setPenWidth(int w);

    void setCurrentSlide(int index);
    int  currentSlide() const;

    QImage renderSlideToImage(int w, int h) const;   // for thumbnails

signals:
    void slideChanged(); // emitted when user edits slide

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;

private:
    Tool tool_ = Tool::Draw;
    QColor penColor_ = Qt::black;
    int penWidth_ = 3;

    bool drawing_ = false;
    QPoint lastPos_;

    int slideIndex_ = 0;

private:
    // Model ↔ QImage glue
    QImage loadInkLayer(int slideIndex) const;
    void   saveInkLayer(int slideIndex, const QImage& img);

    void   drawSlide(QPainter& p, const QRect& r) const;
    QRect  slideRect() const;

    QPoint toSlideSpace(const QPoint& widgetPos) const;
};
