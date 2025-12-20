#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QString>
#include <optional>

#include "Slide.hpp"   // from /include
#include "Shape.hpp"   // ShapeKind + ::Shape

class CanvasView : public QGraphicsView {
    Q_OBJECT
public:
    explicit CanvasView(QWidget* parent = nullptr);

    // Rendering
    void showMessage(const QString& msg);
    void renderSlide(const Slide& slide);

    // Features expected by MainWindow.cpp
    void setShowGrid(bool on);
    bool showGrid() const { return showGrid_; }

    void zoomIn();
    void zoomOut();
    void zoomReset();

private:
    QGraphicsScene* scene_ = nullptr;

    bool showGrid_ = false;
    double zoom_ = 1.0;

    // keep a copy so toggles (grid/zoom reset) can redraw the same content
    std::optional<Slide> lastSlide_;

private:
    void clearScene();
    void drawSlideFrame();
    void drawGrid();
    void applyZoom();
};
