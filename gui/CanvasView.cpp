#include "gui/CanvasView.hpp"

#include "Slide.hpp"
#include "Shape.hpp"

#include <QMouseEvent>
#include <QVariant>
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
static QGraphicsItem* itemWithIndex(QGraphicsItem* it)
{
    while (it) {
        QVariant v = it->data(0);
        if (v.isValid())
            return it;
        it = it->parentItem();
    }
    return nullptr;
}


static int toEmu(int px) { 
    return px * 9525; 
}
}


void CanvasView::mousePressEvent(QMouseEvent* event)
{
    pressedItem_ = nullptr;
    pressedIndex_ = -1;
    pressedPos_ = QPointF();

    if (event && event->button() == Qt::LeftButton) {
        QGraphicsItem* hit = itemAt(event->pos());
        QGraphicsItem* withIdx = itemWithIndex(hit);
        if (withIdx) {
            pressedItem_ = withIdx;
            pressedIndex_ = withIdx->data(0).toInt();
            pressedPos_ = withIdx->pos();
        }
    }

    QGraphicsView::mousePressEvent(event);
}

void CanvasView::mouseReleaseEvent(QMouseEvent* event)
{
    QGraphicsView::mouseReleaseEvent(event);

    if (event && event->button() == Qt::LeftButton && pressedItem_ && pressedIndex_ >= 0) {
        const QPointF newPos = pressedItem_->pos();
        if ((newPos - pressedPos_).manhattanLength() > 0.1) {
            const int x = static_cast<int>(std::lround(newPos.x()));
            const int y = static_cast<int>(std::lround(newPos.y()));
            emit shapeMoved(pressedIndex_, x, y);
        }
    }

    pressedItem_ = nullptr;
    pressedIndex_ = -1;
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

        if (sh.isImage()) {
            const auto& data = sh.getImageData();
            QByteArray bytes(reinterpret_cast<const char*>(data.data()),
                             static_cast<int>(data.size()));

            QImage img = QImage::fromData(bytes);
            if (img.isNull()) {
                auto* err = scene_->addText("[Image decode failed]");
                err->setPos(sh.getX(), sh.getY());
                err->setData(0, i);
                err->setFlag(QGraphicsItem::ItemIsSelectable, true);
                err->setFlag(QGraphicsItem::ItemIsMovable, true);
                continue;
            }

            auto* item = scene_->addPixmap(QPixmap::fromImage(img));
            item->setPos(sh.getX(), sh.getY());
            item->setData(0, i);
            item->setFlag(QGraphicsItem::ItemIsSelectable, true);
            item->setFlag(QGraphicsItem::ItemIsMovable, true);
            continue;
        }

        if (sh.kind() == ShapeKind::Rect || sh.kind() == ShapeKind::Ellipse) {
            QGraphicsItem* base = nullptr;

            QPen pen(Qt::black);
            QBrush brush(QColor(230, 230, 230));

            if (sh.kind() == ShapeKind::Rect) {
                auto* r = scene_->addRect(0, 0, sh.getW(), sh.getH(), pen, brush);
                base = r;
            } else {
                auto* e = scene_->addEllipse(0, 0, sh.getW(), sh.getH(), pen, brush);
                base = e;
            }

            // IMPORTANT: position the item with setPos(), do NOT bake x/y into the local geometry
            base->setPos(sh.getX(), sh.getY());

            base->setData(0, i);
            base->setFlag(QGraphicsItem::ItemIsSelectable, true);
            base->setFlag(QGraphicsItem::ItemIsMovable, true);

            QString text = QString::fromStdString(sh.getText());
            if (!text.isEmpty()) {
                auto* t = new QGraphicsTextItem(text);
                t->setParentItem(base);

                QFont f = t->font();
                f.setPointSize(14);
                t->setFont(f);

                t->setDefaultTextColor(Qt::black);
                t->setPos(10, 8);

                // Let clicks/drags go to the parent shape (the rect/ellipse)
                t->setAcceptedMouseButtons(Qt::NoButton);
                t->setFlag(QGraphicsItem::ItemIsSelectable, false);
            }
            continue;
        }


        QString text = QString::fromStdString(sh.getText());
        auto* item = scene_->addText(text);

        QFont f = item->font();
        f.setPointSize(16);
        item->setFont(f);

        item->setPos(sh.getX(), sh.getY());
        item->setData(0, i);
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        item->setFlag(QGraphicsItem::ItemIsMovable, true);
    }
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
