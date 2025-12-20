#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QString>
#include <QPointF>

#include "Slide.hpp" 

class CanvasView : public QGraphicsView {
    Q_OBJECT
public:
    enum class Tool {
        Select,
        Text,
        Rect,
        Ellipse,
        PlaceImage
    };

    explicit CanvasView(QWidget* parent = nullptr);

    void showMessage(const QString& msg);
    void renderSlide(const ::Slide& slide);

    void setShowGrid(bool on);
    void setSnapToGrid(bool on);

    void zoomIn();
    void zoomOut();
    void zoomReset();

    void setTool(CanvasView::Tool t);
    CanvasView::Tool tool() const { return tool_; }

    void beginPlaceImage(std::string name, std::vector<uint8_t> bytes);

    void deleteSelection();
    void duplicateSelection();

    void commitShapePos(int idx, const QPointF& pos);
    void commitShapeSize(int idx, int w, int h);
    void commitShapeText(int idx, const QString& t);

    int snapInt(int v) const;

protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override;

    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    void keyPressEvent(QKeyEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;

private:
    void ensureScene();
    void clearScene();

    ::Slide* currentSlide(); 
    QPointF snapPoint(const QPointF& p) const;

private:
    QGraphicsScene* scene_ = nullptr;

    bool showGrid_ = false;
    bool snapToGrid_ = false;

    double zoom_ = 1.0;

    Tool tool_ = Tool::Select;

    QGraphicsItem* rubber_ = nullptr;
    QPointF dragStartScene_;
    bool draggingNew_ = false;

    std::string pendingImageName_;
    std::vector<uint8_t> pendingImageBytes_;
};
