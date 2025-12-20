#include "gui/CanvasView.hpp"

#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsLineItem>
#include <QImage>
#include <QPixmap>
#include <QPen>
#include <QBrush>
#include <QTransform>

static constexpr int SLIDE_W = 960;
static constexpr int SLIDE_H = 540;

static constexpr int GRID_STEP = 20;   // px
static constexpr int GRID_BOLD = 100;  // px

CanvasView::CanvasView(QWidget* parent)
    : QGraphicsView(parent)
{
    scene_ = new QGraphicsScene(this);
    setScene(scene_);

    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);

    scene_->setSceneRect(0, 0, SLIDE_W, SLIDE_H);

    // nicer UX
    setDragMode(QGraphicsView::ScrollHandDrag);
}

void CanvasView::applyZoom()
{
    if (zoom_ < 0.1) zoom_ = 0.1;
    if (zoom_ > 10.0) zoom_ = 10.0;

    QTransform t;
    t.scale(zoom_, zoom_);
    setTransform(t);
}

void CanvasView::zoomIn()
{
    zoom_ *= 1.10;
    applyZoom();
}

void CanvasView::zoomOut()
{
    zoom_ /= 1.10;
    applyZoom();
}

void CanvasView::zoomReset()
{
    zoom_ = 1.0;
    applyZoom();
}

void CanvasView::setShowGrid(bool on)
{
    showGrid_ = on;

    // Re-render current view
    if (lastSlide_) {
        renderSlide(*lastSlide_);
    } else {
        showMessage("No presentation. Use toolbar -> New Slide\nor command: create slideshow Name");
    }
}

void CanvasView::clearScene()
{
    scene_->clear();
    scene_->setSceneRect(0, 0, SLIDE_W, SLIDE_H);
}

void CanvasView::drawSlideFrame()
{
    // White slide background with border
    auto* bg = scene_->addRect(
        QRectF(0, 0, SLIDE_W, SLIDE_H),
        QPen(Qt::black, 1.0),
        QBrush(Qt::white)
    );
    bg->setZValue(-1000);
}

void CanvasView::drawGrid()
{
    if (!showGrid_) return;

    // Light grid lines
    QPen light(Qt::lightGray, 0); // cosmetic pen
    // Bold grid lines
    QPen bold(Qt::gray, 0);

    // vertical lines
    for (int x = 0; x <= SLIDE_W; x += GRID_STEP) {
        bool isBold = (x % GRID_BOLD) == 0;
        auto* ln = scene_->addLine(x, 0, x, SLIDE_H, isBold ? bold : light);
        ln->setZValue(-900);
    }

    // horizontal lines
    for (int y = 0; y <= SLIDE_H; y += GRID_STEP) {
        bool isBold = (y % GRID_BOLD) == 0;
        auto* ln = scene_->addLine(0, y, SLIDE_W, y, isBold ? bold : light);
        ln->setZValue(-900);
    }
}

void CanvasView::showMessage(const QString& msg)
{
    lastSlide_.reset();

    clearScene();
    drawSlideFrame();
    drawGrid();

    auto* t = scene_->addText(msg);
    QRectF br = t->boundingRect();
    t->setPos((SLIDE_W - br.width()) * 0.5, (SLIDE_H - br.height()) * 0.5);
    t->setZValue(10);

    applyZoom();
}

void CanvasView::renderSlide(const Slide& slide)
{
    // store copy so toggles can redraw without MainWindow doing extra work
    lastSlide_ = slide;

    clearScene();
    drawSlideFrame();
    drawGrid();

    if (slide.isEmpty()) {
        auto* t = scene_->addText("Empty slide");
        QRectF br = t->boundingRect();
        t->setPos((SLIDE_W - br.width()) * 0.5, (SLIDE_H - br.height()) * 0.5);
        t->setZValue(10);
        applyZoom();
        return;
    }

    // IMPORTANT:
    // CanvasView inherits from QFrame, which has QFrame::Shape (enum).
    // If you write "Shape" here without ::, C++ may pick QFrame::Shape.
    for (const ::Shape& sh : slide.getShapes()) {
        const int x = sh.getX();
        const int y = sh.getY();

        // IMAGE
        if (sh.isImage()) {
            const auto& data = sh.getImageData();

            if (data.empty()) {
                auto* err = scene_->addText("[Image: empty data]");
                err->setPos(x, y);
                err->setZValue(20);
                continue;
            }

            QImage img = QImage::fromData(data.data(), static_cast<int>(data.size()));
            if (img.isNull()) {
                auto* err = scene_->addText("[Image: decode failed]");
                err->setPos(x, y);
                err->setZValue(20);
                continue;
            }

            QPixmap pm = QPixmap::fromImage(img);
            auto* item = scene_->addPixmap(pm);
            item->setPos(x, y);
            item->setZValue(5);
            continue;
        }

        // RECT / ELLIPSE
        if (sh.kind() == ShapeKind::Rect || sh.kind() == ShapeKind::Ellipse) {
            int w = sh.getW();
            int h = sh.getH();
            if (w <= 0) w = 220;
            if (h <= 0) h = 80;

            QPen pen(Qt::black, 2.0);
            QBrush brush(Qt::NoBrush);

            if (sh.kind() == ShapeKind::Rect) {
                auto* r = scene_->addRect(QRectF(x, y, w, h), pen, brush);
                r->setZValue(1);
            } else {
                auto* e = scene_->addEllipse(QRectF(x, y, w, h), pen, brush);
                e->setZValue(1);
            }

            // centered text inside
            QString txt = QString::fromStdString(sh.getText());
            if (!txt.trimmed().isEmpty()) {
                auto* t = scene_->addText(txt);
                t->setZValue(10);

                QRectF br = t->boundingRect();
                qreal cx = x + (w - br.width()) * 0.5;
                qreal cy = y + (h - br.height()) * 0.5;
                t->setPos(cx, cy);
            }
            continue;
        }

        // DEFAULT TEXT
        QString text = QString::fromStdString(sh.getName());
        auto* t = scene_->addText(text);
        t->setPos(x, y);
        t->setZValue(10);
    }

    applyZoom();
}
