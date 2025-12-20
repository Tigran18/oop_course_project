#include "gui/CanvasView.hpp"
#include <iostream>

#include "Controller.hpp"
#include "SlideShow.hpp"

#include <QGraphicsTextItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>   // <-- fixes incomplete type errors
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QByteArray>
#include <QImage>
#include <QPixmap>
#include <QFont>
#include <QPalette>
#include <QInputDialog>
#include <QLineEdit>
#include <QKeyEvent>
#include <QWheelEvent>

#include <cmath>
#include <algorithm>

namespace {
    constexpr int kSlideW = 960;
    constexpr int kSlideH = 540;

    constexpr int kGridStep = 25;

    constexpr int kHandle = 8;
    constexpr int kMinW = 30;
    constexpr int kMinH = 20;
}

// ------------------ helper items bound to model ------------------

class ModelTextItem : public QGraphicsTextItem {
public:
    ModelTextItem(CanvasView* v, int idx, const QString& text)
        : view_(v), idx_(idx)
    {
        setPlainText(text);
        setFlag(QGraphicsItem::ItemIsMovable, true);
        setFlag(QGraphicsItem::ItemIsSelectable, true);
        setTextInteractionFlags(Qt::NoTextInteraction);

        setData(0, idx_);
    }

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e) override
    {
        setTextInteractionFlags(Qt::TextEditorInteraction);
        setFocus(Qt::MouseFocusReason);
        QGraphicsTextItem::mouseDoubleClickEvent(e);
    }

    void focusOutEvent(QFocusEvent* e) override
    {
        setTextInteractionFlags(Qt::NoTextInteraction);
        view_->commitShapeText(idx_, toPlainText());
        QGraphicsTextItem::focusOutEvent(e);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* e) override
    {
        QGraphicsTextItem::mouseReleaseEvent(e);
        view_->commitShapePos(idx_, pos());
    }

private:
    CanvasView* view_;
    int idx_;
};

class ModelPixmapItem : public QGraphicsPixmapItem {
public:
    ModelPixmapItem(CanvasView* v, int idx, const QPixmap& pm)
        : view_(v), idx_(idx)
    {
        setPixmap(pm);
        setFlag(QGraphicsItem::ItemIsMovable, true);
        setFlag(QGraphicsItem::ItemIsSelectable, true);

        setData(0, idx_);
    }

protected:
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* e) override
    {
        QGraphicsPixmapItem::mouseReleaseEvent(e);
        view_->commitShapePos(idx_, pos());
    }

private:
    CanvasView* view_;
    int idx_;
};

class ResizableShapeItem : public QGraphicsObject {
public:
    enum class Kind { Rect, Ellipse };

    ResizableShapeItem(CanvasView* v, int idx, Kind k, int w, int h, const QString& text)
        : view_(v), idx_(idx), kind_(k)
    {
        w_ = std::max(w, kMinW);
        h_ = std::max(h, kMinH);

        setFlag(QGraphicsItem::ItemIsMovable, true);
        setFlag(QGraphicsItem::ItemIsSelectable, true);
        setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

        setData(0, idx_);

        textItem_ = new QGraphicsTextItem(text, this);
        textItem_->setTextInteractionFlags(Qt::NoTextInteraction);
        QFont f = textItem_->font();
        f.setPointSize(14);
        textItem_->setFont(f);

        layoutText();
    }

    QRectF boundingRect() const override
    {
        return QRectF(-kHandle, -kHandle, w_ + 2 * kHandle, h_ + 2 * kHandle);
    }

    void paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) override
    {
        p->setRenderHint(QPainter::Antialiasing, true);

        QPen pen(Qt::black);
        pen.setWidth(2);
        p->setPen(pen);
        p->setBrush(Qt::NoBrush);

        QRectF body(0, 0, w_, h_);
        if (kind_ == Kind::Rect) p->drawRect(body);
        else p->drawEllipse(body);

        if (!isSelected()) return;

        p->setBrush(Qt::white);
        for (const QRectF& hr : handleRects())
            p->drawRect(hr);
    }

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e) override
    {
        Q_UNUSED(e);
        bool ok = false;
        QString cur = textItem_->toPlainText();
        QString t = QInputDialog::getText(nullptr, "Edit text", "Text:", QLineEdit::Normal, cur, &ok);
        if (ok) {
            textItem_->setPlainText(t);
            layoutText();
            view_->commitShapeText(idx_, t);
        }
    }

    void mousePressEvent(QGraphicsSceneMouseEvent* e) override
    {
        active_ = hitHandle(e->pos());
        pressPos_ = e->pos();
        pressItemPos_ = pos();
        pressW_ = w_;
        pressH_ = h_;

        if (active_ != Handle::None) {
            resizing_ = true;
            e->accept();
            return;
        }

        resizing_ = false;
        QGraphicsObject::mousePressEvent(e);
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent* e) override
    {
        if (!resizing_) {
            QGraphicsObject::mouseMoveEvent(e);
            return;
        }

        QPointF d = e->pos() - pressPos_;

        double newW = pressW_;
        double newH = pressH_;
        QPointF newPos = pressItemPos_;

        auto clampW = [&](double& W, double& dxMove) {
            if (W < kMinW) { dxMove = pressW_ - kMinW; W = kMinW; }
        };
        auto clampH = [&](double& H, double& dyMove) {
            if (H < kMinH) { dyMove = pressH_ - kMinH; H = kMinH; }
        };

        if (hasLeft(active_)) {
            double dx = d.x();
            newW = pressW_ - dx;
            clampW(newW, dx);
            newPos.setX(pressItemPos_.x() + dx);
        }
        if (hasRight(active_)) {
            double dx = d.x();
            newW = pressW_ + dx;
            if (newW < kMinW) newW = kMinW;
        }
        if (hasTop(active_)) {
            double dy = d.y();
            newH = pressH_ - dy;
            clampH(newH, dy);
            newPos.setY(pressItemPos_.y() + dy);
        }
        if (hasBottom(active_)) {
            double dy = d.y();
            newH = pressH_ + dy;
            if (newH < kMinH) newH = kMinH;
        }

        // snap if enabled
        newPos.setX(view_->snapInt((int)std::lround(newPos.x())));
        newPos.setY(view_->snapInt((int)std::lround(newPos.y())));
        newW = view_->snapInt((int)std::lround(newW));
        newH = view_->snapInt((int)std::lround(newH));
        newW = std::max(newW, (double)kMinW);
        newH = std::max(newH, (double)kMinH);

        prepareGeometryChange();
        setPos(newPos);
        w_ = (int)newW;
        h_ = (int)newH;

        layoutText();
        update();
        e->accept();
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* e) override
    {
        if (resizing_) {
            resizing_ = false;
            view_->commitShapePos(idx_, pos());
            view_->commitShapeSize(idx_, w_, h_);
            e->accept();
            return;
        }

        QGraphicsObject::mouseReleaseEvent(e);
        view_->commitShapePos(idx_, pos());
    }

private:
    enum class Handle { None, TL, T, TR, R, BR, B, BL, L };

    static bool hasLeft(Handle h)   { return h == Handle::TL || h == Handle::L || h == Handle::BL; }
    static bool hasRight(Handle h)  { return h == Handle::TR || h == Handle::R || h == Handle::BR; }
    static bool hasTop(Handle h)    { return h == Handle::TL || h == Handle::T || h == Handle::TR; }
    static bool hasBottom(Handle h) { return h == Handle::BL || h == Handle::B || h == Handle::BR; }

    QVector<QRectF> handleRects() const
    {
        const double s = kHandle;
        const QRectF b(0, 0, w_, h_);

        const QPointF tl = b.topLeft();
        const QPointF tr = b.topRight();
        const QPointF bl = b.bottomLeft();
        const QPointF br = b.bottomRight();
        const QPointF tm((tl.x() + tr.x()) * 0.5, tl.y());
        const QPointF bm((bl.x() + br.x()) * 0.5, bl.y());
        const QPointF lm(tl.x(), (tl.y() + bl.y()) * 0.5);
        const QPointF rm(tr.x(), (tr.y() + br.y()) * 0.5);

        auto mk = [&](QPointF c) { return QRectF(c.x() - s * 0.5, c.y() - s * 0.5, s, s); };

        return { mk(tl), mk(tm), mk(tr), mk(rm), mk(br), mk(bm), mk(bl), mk(lm) };
    }

    Handle hitHandle(const QPointF& p) const
    {
        if (!isSelected()) return Handle::None;

        auto hs = handleRects();
        if (hs[0].contains(p)) return Handle::TL;
        if (hs[1].contains(p)) return Handle::T;
        if (hs[2].contains(p)) return Handle::TR;
        if (hs[3].contains(p)) return Handle::R;
        if (hs[4].contains(p)) return Handle::BR;
        if (hs[5].contains(p)) return Handle::B;
        if (hs[6].contains(p)) return Handle::BL;
        if (hs[7].contains(p)) return Handle::L;
        return Handle::None;
    }

    void layoutText()
    {
        if (!textItem_) return;

        textItem_->setTextWidth(std::max(0, w_ - 10));

        QRectF tb = textItem_->boundingRect();
        const double tx = (w_ - tb.width()) * 0.5;
        const double ty = (h_ - tb.height()) * 0.5;
        textItem_->setPos(tx, ty);
    }

private:
    CanvasView* view_;
    int idx_;
    Kind kind_;

    int w_ = 220;
    int h_ = 80;

    QGraphicsTextItem* textItem_ = nullptr;

    bool resizing_ = false;
    Handle active_ = Handle::None;

    QPointF pressPos_;
    QPointF pressItemPos_;
    double pressW_ = 0;
    double pressH_ = 0;
};

// ------------------ CanvasView ------------------

CanvasView::CanvasView(QWidget* parent)
    : QGraphicsView(parent)
{
    ensureScene();

    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::TextAntialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);

    setDragMode(QGraphicsView::RubberBandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    setFocusPolicy(Qt::StrongFocus);

    showMessage("No presentation.\nUse toolbar -> New Slide\nor command: create slideshow Name");
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

::Slide* CanvasView::currentSlide()
{
    auto& ctrl = Controller::instance();
    if (ctrl.getSlideshows().empty()) return nullptr;

    auto& ss = ctrl.getCurrentSlideshow();
    auto& slides = ss.getSlides();
    if (slides.empty()) return nullptr;

    size_t idx = ss.getCurrentIndex();
    if (idx >= slides.size()) return nullptr;

    return &slides[idx];
}

void CanvasView::renderSlide(const ::Slide& slide)
{
    clearScene();
    scene_->setSceneRect(0, 0, kSlideW, kSlideH);

    auto* page = scene_->addRect(0, 0, kSlideW, kSlideH, QPen(Qt::black), QBrush(Qt::white));
    page->setZValue(-10);

    if (slide.isEmpty())
        return;

    const auto& shapes = slide.getShapes();

    for (int i = 0; i < (int)shapes.size(); ++i) {
        const ::Shape& sh = shapes[i];

        if (sh.kind() == ::ShapeKind::Image) {
            const auto& data = sh.getImageData();
            QByteArray bytes(reinterpret_cast<const char*>(data.data()),
                             static_cast<int>(data.size()));

            QImage img = QImage::fromData(bytes);
            if (img.isNull()) {
                auto* err = scene_->addText("[Image decode failed]");
                err->setPos(sh.getX(), sh.getY());
                continue;
            }

            auto* item = new ModelPixmapItem(this, i, QPixmap::fromImage(img));
            scene_->addItem(item);
            item->setPos(sh.getX(), sh.getY());
            continue;
        }

        if (sh.kind() == ::ShapeKind::Rect || sh.kind() == ::ShapeKind::Ellipse) {
            auto kind = (sh.kind() == ::ShapeKind::Rect)
                        ? ResizableShapeItem::Kind::Rect
                        : ResizableShapeItem::Kind::Ellipse;

            auto* item = new ResizableShapeItem(
                this, i, kind, sh.getW(), sh.getH(),
                QString::fromStdString(sh.getText())
            );
            scene_->addItem(item);
            item->setPos(sh.getX(), sh.getY());
            continue;
        }

        auto* t = new ModelTextItem(this, i, QString::fromStdString(sh.getText()));
        scene_->addItem(t);
        QFont f = t->font();
        f.setPointSize(16);
        t->setFont(f);
        t->setPos(sh.getX(), sh.getY());
    }
}

void CanvasView::setShowGrid(bool on)
{
    showGrid_ = on;
    viewport()->update();
}

void CanvasView::setSnapToGrid(bool on)
{
    snapToGrid_ = on;
}

int CanvasView::snapInt(int v) const
{
    if (!snapToGrid_) return v;
    int r = (int)std::lround((double)v / (double)kGridStep) * kGridStep;
    return r;
}

QPointF CanvasView::snapPoint(const QPointF& p) const
{
    if (!snapToGrid_) return p;
    return QPointF(snapInt((int)std::lround(p.x())), snapInt((int)std::lround(p.y())));
}

void CanvasView::setTool(CanvasView::Tool t)
{
    tool_ = t;

    if (rubber_) {
        scene_->removeItem(rubber_);
        delete rubber_;
        rubber_ = nullptr;
    }
    draggingNew_ = false;
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

void CanvasView::beginPlaceImage(std::string name, std::vector<uint8_t> bytes)
{
    pendingImageName_ = std::move(name);
    pendingImageBytes_ = std::move(bytes);
    setTool(Tool::PlaceImage);
}

void CanvasView::commitShapePos(int idx, const QPointF& pos)
{
    ::Slide* s = currentSlide();
    if (!s) return;

    auto& vec = s->getShapes();
    if (idx < 0 || idx >= (int)vec.size()) return;

    QPointF p = snapPoint(pos);

    Controller::instance().snapshot();
    vec[idx].setPos((int)std::lround(p.x()), (int)std::lround(p.y()));
}

void CanvasView::commitShapeSize(int idx, int w, int h)
{
    ::Slide* s = currentSlide();
    if (!s) return;

    auto& vec = s->getShapes();
    if (idx < 0 || idx >= (int)vec.size()) return;

    w = std::max(snapInt(w), kMinW);
    h = std::max(snapInt(h), kMinH);

    Controller::instance().snapshot();
    vec[idx].setSize(w, h);
}

void CanvasView::commitShapeText(int idx, const QString& t)
{
    ::Slide* s = currentSlide();
    if (!s) return;

    auto& vec = s->getShapes();
    if (idx < 0 || idx >= (int)vec.size()) return;

    Controller::instance().snapshot();
    vec[idx].setText(t.toStdString());
}

void CanvasView::deleteSelection()
{
    ::Slide* s = currentSlide();
    if (!s) return;

    auto sel = scene_->selectedItems();
    if (sel.isEmpty()) return;

    std::vector<int> idxs;
    idxs.reserve(sel.size());
    for (auto* it : sel) {
        bool ok = false;
        int idx = it->data(0).toInt(&ok);
        if (ok) idxs.push_back(idx);
    }

    if (idxs.empty()) return;

    std::sort(idxs.begin(), idxs.end());
    idxs.erase(std::unique(idxs.begin(), idxs.end()), idxs.end());

    auto& vec = s->getShapes();
    Controller::instance().snapshot();

    // erase from back to front
    for (int k = (int)idxs.size() - 1; k >= 0; --k) {
        int i = idxs[k];
        if (i >= 0 && i < (int)vec.size())
            vec.erase(vec.begin() + i);
    }

    renderSlide(*s);
}

void CanvasView::duplicateSelection()
{
    ::Slide* s = currentSlide();
    if (!s) return;

    auto sel = scene_->selectedItems();
    if (sel.isEmpty()) return;

    std::vector<int> idxs;
    for (auto* it : sel) {
        bool ok = false;
        int idx = it->data(0).toInt(&ok);
        if (ok) idxs.push_back(idx);
    }
    if (idxs.empty()) return;

    std::sort(idxs.begin(), idxs.end());
    idxs.erase(std::unique(idxs.begin(), idxs.end()), idxs.end());

    auto& vec = s->getShapes();
    Controller::instance().snapshot();

    for (int idx : idxs) {
        if (idx < 0 || idx >= (int)vec.size()) continue;
        ::Shape copy = vec[idx];
        copy.setPos(copy.getX() + 20, copy.getY() + 20);
        vec.push_back(copy);
    }

    renderSlide(*s);
}

void CanvasView::mousePressEvent(QMouseEvent* e)
{
    const QPointF scenePos = mapToScene(e->pos());

    if (tool_ == Tool::Text && e->button() == Qt::LeftButton) {
        ::Slide* s = currentSlide();
        if (!s) return;

        bool ok = false;
        QString t = QInputDialog::getText(this, "Insert Text", "Text:", QLineEdit::Normal, "", &ok);
        if (!ok) { setTool(Tool::Select); return; }

        QPointF p = snapPoint(scenePos);

        Controller::instance().snapshot();
        s->addShape(::Shape(t.toStdString(), (int)p.x(), (int)p.y()));
        std::cout << "[INFO] Added text shape\n";

        renderSlide(*s);
        setTool(Tool::Select);
        return;
    }

    if (tool_ == Tool::PlaceImage && e->button() == Qt::LeftButton) {
        ::Slide* s = currentSlide();
        if (!s) return;

        if (pendingImageBytes_.empty()) { setTool(Tool::Select); return; }

        QPointF p = snapPoint(scenePos);

        Controller::instance().snapshot();
        s->addShape(::Shape(
            pendingImageName_.empty() ? "Image" : pendingImageName_,
            (int)p.x(), (int)p.y(),
            pendingImageBytes_
        ));
        std::cout << "[INFO] Added image\n";

        pendingImageBytes_.clear();
        pendingImageName_.clear();

        renderSlide(*s);
        setTool(Tool::Select);
        return;
    }

    if ((tool_ == Tool::Rect || tool_ == Tool::Ellipse) && e->button() == Qt::LeftButton) {
        draggingNew_ = true;
        dragStartScene_ = scenePos;

        if (rubber_) {
            scene_->removeItem(rubber_);
            delete rubber_;
            rubber_ = nullptr;
        }

        QPen pen(Qt::black);
        pen.setWidth(2);

        if (tool_ == Tool::Rect)
            rubber_ = scene_->addRect(QRectF(scenePos, scenePos), pen, Qt::NoBrush);
        else
            rubber_ = scene_->addEllipse(QRectF(scenePos, scenePos), pen, Qt::NoBrush);

        rubber_->setZValue(1000);
        e->accept();
        return;
    }

    QGraphicsView::mousePressEvent(e);
}

void CanvasView::mouseMoveEvent(QMouseEvent* e)
{
    if (!draggingNew_ || !rubber_) {
        QGraphicsView::mouseMoveEvent(e);
        return;
    }

    QPointF cur = mapToScene(e->pos());
    QRectF r(dragStartScene_, cur);
    r = r.normalized();

    if (auto* ri = dynamic_cast<QGraphicsRectItem*>(rubber_))
        ri->setRect(r);
    else if (auto* ei = dynamic_cast<QGraphicsEllipseItem*>(rubber_))
        ei->setRect(r);

    e->accept();
}

void CanvasView::mouseReleaseEvent(QMouseEvent* e)
{
    if (!draggingNew_ || !rubber_) {
        QGraphicsView::mouseReleaseEvent(e);
        return;
    }

    draggingNew_ = false;

    QRectF r;
    if (auto* ri = dynamic_cast<QGraphicsRectItem*>(rubber_)) r = ri->rect();
    if (auto* ei = dynamic_cast<QGraphicsEllipseItem*>(rubber_)) r = ei->rect();

    scene_->removeItem(rubber_);
    delete rubber_;
    rubber_ = nullptr;

    ::Slide* s = currentSlide();
    if (!s) return;

    r = r.normalized();

    int x = (int)std::lround(r.left());
    int y = (int)std::lround(r.top());
    int w = (int)std::lround(r.width());
    int h = (int)std::lround(r.height());

    x = snapInt(x);
    y = snapInt(y);
    w = std::max(snapInt(w), kMinW);
    h = std::max(snapInt(h), kMinH);

    bool ok = false;
    QString text = QInputDialog::getText(this, "Shape text", "Text:", QLineEdit::Normal, "", &ok);
    if (!ok) text = "";

    std::string raw;
    if (tool_ == Tool::Rect)
        raw = "rect(" + std::to_string(w) + "," + std::to_string(h) + "):" + text.toStdString();
    else
        raw = "ellipse(" + std::to_string(w) + "," + std::to_string(h) + "):" + text.toStdString();

    Controller::instance().snapshot();
    s->addShape(::Shape(raw, x, y));
    std::cout << "[INFO] Added " << (tool_ == Tool::Rect ? "rectangle" : "ellipse") << "\n";

    renderSlide(*s);
    setTool(Tool::Select);
    e->accept();
}

void CanvasView::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Delete) {
        deleteSelection();
        e->accept();
        return;
    }

    if ((e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_D) {
        duplicateSelection();
        e->accept();
        return;
    }

    QGraphicsView::keyPressEvent(e);
}

void CanvasView::wheelEvent(QWheelEvent* e)
{
    if (e->modifiers() & Qt::ControlModifier) {
        if (e->angleDelta().y() > 0) zoomIn();
        else zoomOut();
        e->accept();
        return;
    }
    QGraphicsView::wheelEvent(e);
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

    const int left   = (int)std::floor(rect.left());
    const int right  = (int)std::ceil(rect.right());
    const int top    = (int)std::floor(rect.top());
    const int bottom = (int)std::ceil(rect.bottom());

    int firstX = left - (left % kGridStep);
    int firstY = top  - (top  % kGridStep);

    for (int x = firstX; x < right; x += kGridStep)
        painter->drawLine(x, top, x, bottom);

    for (int y = firstY; y < bottom; y += kGridStep)
        painter->drawLine(left, y, right, y);

    painter->restore();
}
