#pragma once

#include <QGraphicsView>

class QGraphicsScene;
class Slide;

class CanvasView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit CanvasView(QWidget* parent = nullptr);

    void showMessage(const QString& msg);
    void renderSlide(const ::Slide& slide);

    void setShowGrid(bool on);

    // Called by internal graphics items when the user drags a shape.
    // Emits shapeMoved() with integer pixel coordinates in canvas space.
    void notifyShapeMoved(int index, const QPointF& newPos);

public slots:
    void zoomIn();
    void zoomOut();
    void zoomReset();

signals:
    void shapeSelected(int index);
    void selectionCleared();

    // Fired once after a drag finishes (mouse release) if the shape moved.
    void shapeMoved(int index, int x, int y);

protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override;

private:
    void ensureScene();
    void clearScene();

private:
    QGraphicsScene* scene_ = nullptr;
    bool showGrid_ = false;
    double zoom_ = 1.0;
};
