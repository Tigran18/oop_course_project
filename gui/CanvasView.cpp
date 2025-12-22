#include "gui/CanvasView.hpp"

#include "Slide.hpp"
#include "Shape.hpp"

#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPixmapItem>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QByteArray>
#include <QImage>
#include <QPixmap>
#include <QFont>
#include <QPalette>
#include <cmath>

namespace {
constexpr int kSlideW = 960;
constexpr int kSlideH = 540;
constexpr int kGridStep = 25;

// A movable/selectable graphics item that reports its final position to CanvasView
// on mouse release. We keep the "shape index" in data(0) so selection works.
class TaggedRectItem : public QGraphicsRectItem {
public:
    TaggedRectItem(int idx, const QRectF& r, CanvasView* view)
        : QGraphicsRectItem(r), idx_(idx), view_(view) {
        setData(0, idx_);
        setFlag(QGraphicsItem::ItemIsSelectable, true);
        setFlag(QGraphicsItem::ItemIsMovable, true);
    }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* e) override {
        startPos_ = pos();
        QGraphicsRectItem::mousePressEvent(e);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* e) override {
        QGraphicsRectItem::mouseReleaseEvent(e);
        if (view_ && pos() != startPos_) view_->notifyShapeMoved(idx_, pos());
    }

private:
    int idx_ = -1;
    CanvasView* view_ = nullptr;
    QPointF startPos_;
};

class TaggedEllipseItem : public QGraphicsEllipseItem {
public:
    TaggedEllipseItem(int idx, const QRectF& r, CanvasView* view)
        : QGraphicsEllipseItem(r), idx_(idx), view_(view) {
        setData(0, idx_);
        setFlag(QGraphicsItem::ItemIsSelectable, true);
        setFlag(QGraphicsItem::ItemIsMovable, true);
    }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* e) override {
        startPos_ = pos();
        QGraphicsEllipseItem::mousePressEvent(e);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* e) override {
        QGraphicsEllipseItem::mouseReleaseEvent(e);
        if (view_ && pos() != startPos_) view_->notifyShapeMoved(idx_, pos());
    }

private:
    int idx_ = -1;
    CanvasView* view_ = nullptr;
    QPointF startPos_;
};

class TaggedPixmapItem : public QGraphicsPixmapItem {
public:
    TaggedPixmapItem(int idx, const QPixmap& pm, CanvasView* view)
        : QGraphicsPixmapItem(pm), idx_(idx), view_(view) {
        setData(0, idx_);
        setFlag(QGraphicsItem::ItemIsSelectable, true);
        setFlag(QGraphicsItem::ItemIsMovable, true);
    }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* e) override {
        startPos_ = pos();
        QGraphicsPixmapItem::mousePressEvent(e);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* e) override {
        QGraphicsPixmapItem::mouseReleaseEvent(e);
        if (view_ && pos() != startPos_) view_->notifyShapeMoved(idx_, pos());
    }

private:
    int idx_ = -1;
    CanvasView* view_ = nullptr;
    QPointF startPos_;
};

class TaggedTextItem : public QGraphicsTextItem {
public:
    TaggedTextItem(int idx, const QString& text, CanvasView* view)
        : QGraphicsTextItem(text), idx_(idx), view_(view) {
        setData(0, idx_);
        setFlag(QGraphicsItem::ItemIsSelectable, true);
        setFlag(QGraphicsItem::ItemIsMovable, true);
    }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* e) override {
        startPos_ = pos();
        QGraphicsTextItem::mousePressEvent(e);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* e) override {
        QGraphicsTextItem::mouseReleaseEvent(e);
        if (view_ && pos() != startPos_) view_->notifyShapeMoved(idx_, pos());
    }

private:
    int idx_ = -1;
    CanvasView* view_ = nullptr;
    QPointF startPos_;
};
}

CanvasView::CanvasView(QWidget* parent)
    : QGraphicsView(parent)
{
    ensureScene();

    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::TextAntialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);

    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    connect(scene_, &QGraphicsScene::selectionChanged, this, [this]() {
        const auto items = scene_->selectedItems();
        if (items.isEmpty()) {
            emit selectionCleared();
            return;
        }
        QGraphicsItem* it = items.first();
        QVariant v = it->data(0);
        if (v.isValid()) {
            emit shapeSelected(v.toInt());
            return;
        }
        emit selectionCleared();
    });

    showMessage("No presentation.\nUse File -> New Presentation");
}

void CanvasView::ensureScene()
{
    if (!scene_) {
        scene_ = new QGraphicsScene(this);
        setScene(scene_);
    }
}

void CanvasView::clearScene()
{
    ensureScene();
    scene_->clear();
}

void CanvasView::showMessage(const QString& msg)
{
    clearScene();
    scene_->setSceneRect(0, 0, kSlideW, kSlideH);

    auto* page = scene_->addRect(0, 0, kSlideW, kSlideH, QPen(Qt::black), QBrush(Qt::white));
    page->setZValue(-10);

    auto* t = scene_->addText(msg);
    QFont f = t->font();
    f.setPointSize(12);
    t->setFont(f);

    QRectF r = t->boundingRect();
    t->setPos((kSlideW - r.width()) * 0.5, (kSlideH - r.height()) * 0.5);
}

void CanvasView::renderSlide(const ::Slide& slide)
{
    clearScene();
    scene_->setSceneRect(0, 0, kSlideW, kSlideH);

    auto* page = scene_->addRect(0, 0, kSlideW, kSlideH, QPen(Qt::black), QBrush(Qt::white));
    page->setZValue(-10);

    const auto& shapes = slide.getShapes();
    if (shapes.empty()) {
        auto* t = scene_->addText("Empty slide");
        t->setPos(30, 30);
        return;
    }

    for (int i = 0; i < (int)shapes.size(); ++i) {
        const ::Shape& sh = shapes[(size_t)i];

        // -------------------------
        // Images
        // -------------------------
        if (sh.isImage()) {
            const auto& data = sh.getImageData();
            QByteArray bytes(reinterpret_cast<const char*>(data.data()),
                             static_cast<int>(data.size()));

            QImage img = QImage::fromData(bytes);
            if (img.isNull()) {
                auto* err = new TaggedTextItem(i, "[Image decode failed]", this);
                err->setPos(sh.getX(), sh.getY());
                scene_->addItem(err);
                continue;
            }

            int targetW = (sh.getW() > 1) ? sh.getW() : img.width();
            int targetH = (sh.getH() > 1) ? sh.getH() : img.height();
            if (targetW <= 0) targetW = 1;
            if (targetH <= 0) targetH = 1;

            // Keep images reasonable inside the slide view.
            const int maxW = kSlideW;
            const int maxH = kSlideH;
            if (targetW > maxW || targetH > maxH) {
                double sx = (double)maxW / (double)targetW;
                double sy = (double)maxH / (double)targetH;
                double s = std::min(sx, sy);
                targetW = std::max(1, (int)std::round(targetW * s));
                targetH = std::max(1, (int)std::round(targetH * s));
            }

            QPixmap pm = QPixmap::fromImage(img);
            if (pm.width() != targetW || pm.height() != targetH) {
                pm = pm.scaled(targetW, targetH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }

            auto* item = new TaggedPixmapItem(i, pm, this);
            item->setPos(sh.getX(), sh.getY());
            scene_->addItem(item);
            continue;
        }

        // -------------------------
        // Rectangles / Ellipses
        // -------------------------
        if (sh.kind() == ShapeKind::Rect || sh.kind() == ShapeKind::Ellipse) {
            QPen pen(Qt::black);
            QBrush brush(QColor(230, 230, 230));

            const QRectF localRect(0, 0,
                                   std::max(1, sh.getW()),
                                   std::max(1, sh.getH()));

            QGraphicsItem* base = nullptr;
            if (sh.kind() == ShapeKind::Rect) {
                auto* r = new TaggedRectItem(i, localRect, this);
                r->setPen(pen);
                r->setBrush(brush);
                r->setPos(sh.getX(), sh.getY());
                scene_->addItem(r);
                base = r;
            } else {
                auto* e = new TaggedEllipseItem(i, localRect, this);
                e->setPen(pen);
                e->setBrush(brush);
                e->setPos(sh.getX(), sh.getY());
                scene_->addItem(e);
                base = e;
            }

            // Put the label INSIDE the shape (local coordinates).
            QString text = QString::fromStdString(sh.getText());
            if (!text.isEmpty()) {
                auto* t = new QGraphicsTextItem(text, base);
                QFont f = t->font();
                f.setPointSize(14);
                t->setFont(f);
                t->setDefaultTextColor(Qt::black);
                t->setPos(10, 8);
                t->setFlag(QGraphicsItem::ItemIsSelectable, false);
                t->setFlag(QGraphicsItem::ItemIsMovable, false);
                t->setAcceptedMouseButtons(Qt::NoButton);
            }

            continue;
        }

        // -------------------------
        // Text
        // -------------------------
        QString text = QString::fromStdString(sh.getText());
        auto* item = new TaggedTextItem(i, text, this);

        QFont f = item->font();
        f.setPointSize(16);
        item->setFont(f);

        item->setPos(sh.getX(), sh.getY());
        scene_->addItem(item);
    }
}

void CanvasView::notifyShapeMoved(int index, const QPointF& newPos)
{
    // Convert to integer canvas pixels.
    const int x = static_cast<int>(std::round(newPos.x()));
    const int y = static_cast<int>(std::round(newPos.y()));
    emit shapeMoved(index, x, y);
}

void CanvasView::setShowGrid(bool on)
{
    showGrid_ = on;
    viewport()->update();
}

void CanvasView::zoomIn()
{
    constexpr double factor = 1.15;
    zoom_ *= factor;
    scale(factor, factor);
}

void CanvasView::zoomOut()
{
    constexpr double factor = 1.15;
    zoom_ /= factor;
    scale(1.0 / factor, 1.0 / factor);
}

void CanvasView::zoomReset()
{
    zoom_ = 1.0;
    setTransform(QTransform());
}

void CanvasView::drawBackground(QPainter* painter, const QRectF& rect)
{
    QGraphicsView::drawBackground(painter, rect);

    if (!showGrid_) return;

    painter->save();

    QColor gridColor = palette().color(QPalette::Midlight);
    gridColor.setAlpha(120);

    QPen pen(gridColor);
    pen.setWidth(0);
    painter->setPen(pen);

    const int left   = static_cast<int>(std::floor(rect.left()));
    const int right  = static_cast<int>(std::ceil(rect.right()));
    const int top    = static_cast<int>(std::floor(rect.top()));
    const int bottom = static_cast<int>(std::ceil(rect.bottom()));

    int firstX = left - (left % kGridStep);
    int firstY = top  - (top  % kGridStep);

    for (int x = firstX; x < right; x += kGridStep)
        painter->drawLine(x, top, x, bottom);

    for (int y = firstY; y < bottom; y += kGridStep)
        painter->drawLine(left, y, right, y);

    painter->restore();
}
