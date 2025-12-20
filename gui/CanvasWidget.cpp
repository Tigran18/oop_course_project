#include "CanvasWidget.hpp"

#include "../include/Controller.hpp"
#include "../include/SlideShow.hpp"
#include "../include/Slide.hpp"
#include "../include/Shape.hpp"

#include <QPainter>
#include <QMouseEvent>
#include <QInputDialog>
#include <QFileDialog>
#include <QBuffer>

static std::vector<uint8_t> qimageToPngBytes(const QImage& img) {
    QByteArray arr;
    QBuffer buf(&arr);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "PNG");
    return std::vector<uint8_t>(arr.begin(), arr.end());
}

static QImage pngBytesToQImage(const std::vector<uint8_t>& data) {
    if (data.empty()) return {};
    QImage img;
    img.loadFromData(reinterpret_cast<const uchar*>(data.data()),
                     int(data.size()),
                     "PNG");
    return img;
}

CanvasWidget::CanvasWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumSize(640, 480);
}

void CanvasWidget::setTool(Tool t) { tool_ = t; }
void CanvasWidget::setPenColor(const QColor& c) { penColor_ = c; }
void CanvasWidget::setPenWidth(int w) { penWidth_ = w; }

void CanvasWidget::setCurrentSlide(int index) {
    slideIndex_ = index;
    update();
}

int CanvasWidget::currentSlide() const { return slideIndex_; }

QRect CanvasWidget::slideRect() const {
    // 4:3 slide-ish inside widget with margins
    int W = width();
    int H = height();
    int margin = 20;

    int availW = W - 2 * margin;
    int availH = H - 2 * margin;

    // 4:3 ratio => width/height = 4/3
    int targetW = availW;
    int targetH = (targetW * 3) / 4;
    if (targetH > availH) {
        targetH = availH;
        targetW = (targetH * 4) / 3;
    }

    int x = (W - targetW) / 2;
    int y = (H - targetH) / 2;
    return QRect(x, y, targetW, targetH);
}

QPoint CanvasWidget::toSlideSpace(const QPoint& widgetPos) const {
    // We treat slide space == pixel space of rendered slide rect
    // If you later want real PPT units, you can map here.
    QRect r = slideRect();
    return QPoint(widgetPos.x() - r.x(), widgetPos.y() - r.y());
}

QImage CanvasWidget::loadInkLayer(int slideIndex) const {
    auto& ctrl = Controller::instance();
    auto& ss = ctrl.getCurrentSlideshow();
    if (ss.getSlides().empty()) return {};

    if (slideIndex < 0 || size_t(slideIndex) >= ss.getSlides().size()) return {};

    const Slide& slide = ss.getSlides()[size_t(slideIndex)];
    for (const auto& sh : slide.getShapes()) {
        if (sh.isImage() && sh.getName() == "__INK__") {
            return pngBytesToQImage(sh.getImageData());
        }
    }
    return {};
}

void CanvasWidget::saveInkLayer(int slideIndex, const QImage& img) {
    auto& ctrl = Controller::instance();
    auto& ss = ctrl.getCurrentSlideshow();
    if (ss.getSlides().empty()) return;
    if (slideIndex < 0 || size_t(slideIndex) >= ss.getSlides().size()) return;

    auto& slide = ss.getSlides()[size_t(slideIndex)];
    auto bytes = qimageToPngBytes(img);

    // replace if exists
    for (auto& sh : slide.getShapes()) {
        if (sh.isImage() && sh.getName() == "__INK__") {
            // overwrite by reconstructing (simple way)
            sh = Shape("__INK__", 0, 0, std::move(bytes));
            emit slideChanged();
            return;
        }
    }

    slide.addShape(Shape("__INK__", 0, 0, std::move(bytes)));
    emit slideChanged();
}

void CanvasWidget::drawSlide(QPainter& p, const QRect& r) const {
    // white background + border
    p.fillRect(r, Qt::white);
    p.setPen(QPen(Qt::gray, 1));
    p.drawRect(r.adjusted(0,0,-1,-1));

    auto& ctrl = Controller::instance();
    auto& ss = ctrl.getCurrentSlideshow();
    if (ss.getSlides().empty()) return;
    if (slideIndex_ < 0 || size_t(slideIndex_) >= ss.getSlides().size()) return;

    const Slide& slide = ss.getSlides()[size_t(slideIndex_)];

    // draw ink first (as background layer)
    {
        QImage ink = loadInkLayer(slideIndex_);
        if (!ink.isNull()) {
            p.drawImage(r, ink);
        }
    }

    // draw text + other images (except __INK__)
    p.setPen(Qt::black);
    for (const auto& sh : slide.getShapes()) {
        if (sh.isImage()) {
            if (sh.getName() == "__INK__") continue;
            QImage img = pngBytesToQImage(sh.getImageData());
            if (!img.isNull()) {
                QPoint pos = QPoint(r.x() + sh.getX(), r.y() + sh.getY());
                p.drawImage(pos, img);
            }
        } else {
            QPoint pos = QPoint(r.x() + sh.getX(), r.y() + sh.getY());
            p.drawText(pos, QString::fromStdString(sh.getName()));
        }
    }
}

void CanvasWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRect r = slideRect();
    drawSlide(p, r);
}

void CanvasWidget::mousePressEvent(QMouseEvent* e) {
    QRect r = slideRect();
    if (!r.contains(e->pos())) return;

    if (tool_ == Tool::Draw && e->button() == Qt::LeftButton) {
        drawing_ = true;
        lastPos_ = e->pos();
    }
}

void CanvasWidget::mouseMoveEvent(QMouseEvent* e) {
    if (!drawing_) return;

    QRect r = slideRect();
    if (!r.contains(e->pos())) return;

    // load current ink image or create a new one matching slide rect size
    QImage ink = loadInkLayer(slideIndex_);
    if (ink.isNull()) {
        ink = QImage(r.size(), QImage::Format_ARGB32_Premultiplied);
        ink.fill(Qt::transparent);
    } else if (ink.size() != r.size()) {
        ink = ink.scaled(r.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    QPainter p(&ink);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(penColor_, penWidth_, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    QPoint a = lastPos_ - r.topLeft();
    QPoint b = e->pos() - r.topLeft();
    p.drawLine(a, b);

    lastPos_ = e->pos();

    // save back immediately (simple and robust)
    saveInkLayer(slideIndex_, ink);
    update();
}

void CanvasWidget::mouseReleaseEvent(QMouseEvent* e) {
    if (drawing_ && e->button() == Qt::LeftButton) {
        drawing_ = false;
    }
}

void CanvasWidget::mouseDoubleClickEvent(QMouseEvent* e) {
    QRect r = slideRect();
    if (!r.contains(e->pos())) return;

    auto& ctrl = Controller::instance();
    auto& ss = ctrl.getCurrentSlideshow();
    if (ss.getSlides().empty()) return;
    if (slideIndex_ < 0 || size_t(slideIndex_) >= ss.getSlides().size()) return;

    QPoint sp = e->pos() - r.topLeft();

    if (tool_ == Tool::Text) {
        bool ok = false;
        QString txt = QInputDialog::getText(this, "Insert Text", "Text:", QLineEdit::Normal, "", &ok);
        if (!ok || txt.isEmpty()) return;

        ctrl.snapshot();
        ss.getSlides()[size_t(slideIndex_)].addShape(
            Shape(txt.toStdString(), sp.x(), sp.y())
        );

        emit slideChanged();
        update();
    }

    if (tool_ == Tool::Image) {
        QString file = QFileDialog::getOpenFileName(this, "Insert Image", "", "Images (*.png *.jpg *.jpeg *.bmp)");
        if (file.isEmpty()) return;

        QImage img(file);
        if (img.isNull()) return;

        // store as PNG bytes
        QByteArray arr;
        QBuffer buf(&arr);
        buf.open(QIODevice::WriteOnly);
        img.save(&buf, "PNG");

        std::vector<uint8_t> bytes(arr.begin(), arr.end());

        ctrl.snapshot();
        ss.getSlides()[size_t(slideIndex_)].addShape(
            Shape("Image", sp.x(), sp.y(), std::move(bytes))
        );

        emit slideChanged();
        update();
    }
}

QImage CanvasWidget::renderSlideToImage(int w, int h) const {
    QImage out(QSize(w, h), QImage::Format_ARGB32_Premultiplied);
    out.fill(Qt::white);

    QPainter p(&out);
    p.setRenderHint(QPainter::Antialiasing);

    QRect r(0, 0, w, h);
    // reuse drawSlide logic but with target rect
    // (we replicate minimal version for thumbnail)
    p.fillRect(r, Qt::white);
    p.setPen(QPen(Qt::gray, 1));
    p.drawRect(r.adjusted(0,0,-1,-1));

    auto& ctrl = Controller::instance();
    auto& ss = ctrl.getCurrentSlideshow();
    if (ss.getSlides().empty()) return out;
    if (slideIndex_ < 0 || size_t(slideIndex_) >= ss.getSlides().size()) return out;

    const Slide& slide = ss.getSlides()[size_t(slideIndex_)];

    // ink
    {
        QImage ink = loadInkLayer(slideIndex_);
        if (!ink.isNull()) {
            p.drawImage(r, ink.scaled(r.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        }
    }

    // text + images (very rough for thumbs)
    p.setPen(Qt::black);
    for (const auto& sh : slide.getShapes()) {
        if (sh.isImage() && sh.getName() != "__INK__") {
            QImage img = pngBytesToQImage(sh.getImageData());
            if (!img.isNull()) {
                // scale coordinates proportionally from current widget slideRect size is unknown here,
                // so simplest: just draw at same x/y but clipped.
                p.drawImage(QPoint(sh.getX()/4, sh.getY()/4), img.scaled(120, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        } else if (!sh.isImage()) {
            p.drawText(QPoint(sh.getX()/4 + 8, sh.getY()/4 + 20),
                       QString::fromStdString(sh.getName()));
        }
    }

    return out;
}
