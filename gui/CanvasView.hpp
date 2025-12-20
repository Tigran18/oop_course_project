#pragma once

#include <QGraphicsView>
#include <QPointF>

class QGraphicsScene;
class Slide;
class QGraphicsItem;
class QMouseEvent;

class CanvasView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit CanvasView(QWidget* parent = nullptr);

    void showMessage(const QString& msg);
    void renderSlide(const ::Slide& slide);

    void setShowGrid(bool on);

public slots:
    void zoomIn();
    void zoomOut();
    void zoomReset();

signals:
    void shapeSelected(int index);
    void selectionCleared();

    // Emitted when a user drags a shape on the canvas
    void shapeMoved(int index, int x, int y);

protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void ensureScene();
    void clearScene();

private:
    QGraphicsScene* scene_ = nullptr;
    bool showGrid_ = false;
    double zoom_ = 1.0;

    // For detecting drag/move and syncing back to the model
    QGraphicsItem* pressedItem_ = nullptr;
    int pressedIndex_ = -1;
    QPointF pressedPos_;
};
