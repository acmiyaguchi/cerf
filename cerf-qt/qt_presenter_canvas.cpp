#include "qt_presenter_canvas.h"

#include "qt_device_pixels.h"

#include <cerffrontend/present_effects.h>

#include <QEvent>
#include <QPainter>
#include <QScrollBar>
#include <QSignalBlocker>

#include <algorithm>

QtPresenterCanvas::QtPresenterCanvas(cerfcore::CerfEmulator& emu, QWidget* parent,
                                     cerfcore::FrameSource& source, uint32_t surf_w,
                                     uint32_t surf_h, int present_interval_ms)
    : QWidget(parent), emu_(emu), source_(source) {
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent);
    hbar_ = new QScrollBar(Qt::Horizontal, this);
    vbar_ = new QScrollBar(Qt::Vertical, this);
    hbar_->hide();
    vbar_->hide();
    hbar_->setSingleStep(cerffrontend::ViewportLayout::kScrollLineStep);
    vbar_->setSingleStep(cerffrontend::ViewportLayout::kScrollLineStep);
    QObject::connect(hbar_, &QScrollBar::valueChanged, this, [this](int v) { OnScroll(true, v); });
    QObject::connect(vbar_, &QScrollBar::valueChanged, this, [this](int v) { OnScroll(false, v); });

    SetGuestSurfaceSize(surf_w, surf_h);
    present_timer_.setTimerType(Qt::PreciseTimer);
    QObject::connect(&present_timer_, &QTimer::timeout, this, [this] { update(); });
    present_timer_.start(present_interval_ms);
}

void QtPresenterCanvas::SetGuestSurfaceSize(uint32_t w, uint32_t h) {
    if (w == 0) w = 1;
    if (h == 0) h = 1;
    if (w == layout_.SurfaceWidth() && h == layout_.SurfaceHeight() && !guest_.isNull()) return;
    guest_ = QImage(static_cast<int>(w), static_cast<int>(h), QImage::Format_RGB32);
    guest_.fill(Qt::black);
    layout_.SetSurfaceSize(w, h);
    layout_.ResetScroll();
    UpdateScrollbars();
    update();
}

void QtPresenterCanvas::SetViewportMode(cerffrontend::ViewportMode m) {
    if (!layout_.SetMode(m)) return;
    UpdateScrollbars();
    update();
}

void QtPresenterCanvas::SetIntegerScale(int factor) {
    if (!layout_.SetIntegerScale(factor)) return;
    UpdateScrollbars();
    update();
}

void QtPresenterCanvas::SetAntialias(bool on) {
    if (layout_.SetAntialias(on)) update();
}

void QtPresenterCanvas::SetFramebufferActive(bool active) {
    if (!layout_.SetActive(active)) return;
    UpdateScrollbars();
    update();
}

bool QtPresenterCanvas::CaptureSurface(std::vector<uint32_t>& out, uint32_t& w, uint32_t& h) {
    if (!source_.HasFrame()) return false;
    const uint32_t sw = layout_.SurfaceWidth();
    const uint32_t sh = layout_.SurfaceHeight();
    if (sw == 0 || sh == 0 || guest_.isNull()) return false;
    auto* bits = reinterpret_cast<uint32_t*>(guest_.bits());
    source_.RenderInto(bits, sw, sh);
    out.assign(bits, bits + static_cast<size_t>(sw) * sh);
    w = sw;
    h = sh;
    return true;
}

bool QtPresenterCanvas::ComposeInto() {
    present_.fill(Qt::black);
    if (!source_.HasFrame() || guest_.isNull()) return false;
    auto* guest_bits = reinterpret_cast<uint32_t*>(guest_.bits());
    source_.RenderInto(guest_bits, layout_.SurfaceWidth(), layout_.SurfaceHeight());
    ForceOpaque(guest_bits, static_cast<size_t>(layout_.SurfaceWidth()) * layout_.SurfaceHeight());

    const cerffrontend::ViewportLayout::Layout L = layout_.Compute();
    if (L.dst_w <= 0 || L.dst_h <= 0) return true;
    QPainter p(&present_);
    if (L.stretch) {
        p.setRenderHint(QPainter::SmoothPixmapTransform, layout_.Antialias());
        p.drawImage(QRect(L.dst_x, L.dst_y, L.dst_w, L.dst_h), guest_,
                    QRect(L.src_x, L.src_y, L.src_w, L.src_h));
    } else {
        p.drawImage(QPoint(L.dst_x, L.dst_y), guest_, QRect(L.src_x, L.src_y, L.dst_w, L.dst_h));
    }
    return true;
}

void QtPresenterCanvas::TickAndPresent() {
    OnPresentTick();
    if (present_.isNull()) return;
    auto*     bits = reinterpret_cast<uint32_t*>(present_.bits());
    const int w    = present_.width();
    const int h    = present_.height();
    if (RenderAlt(bits, w, h)) ForceOpaque(bits, static_cast<size_t>(w) * h);
    else                       ComposeInto();
    if (ShouldDesaturate()) cerffrontend::PresentEffects::DesaturateAndDim(bits, w * h);
    int client_w = 0, client_h = 0;
    VisibleSize(client_w, client_h);
    RenderOverlay(present_, std::min(w, client_w), std::min(h, client_h));
}

void QtPresenterCanvas::paintEvent(QPaintEvent*) {
    TickAndPresent();
    const qreal dpr = Dpr();
    QPainter    p(this);
    p.drawImage(QRectF(0, 0, present_.width() / dpr, present_.height() / dpr), present_);
}

void QtPresenterCanvas::resizeEvent(QResizeEvent*) {
    Relayout();
    if (on_resized_) on_resized_();
}

void QtPresenterCanvas::changeEvent(QEvent* event) {
    if (event->type() == QEvent::DevicePixelRatioChange) {
        Relayout();
        update();
    }
    QWidget::changeEvent(event);
}

void QtPresenterCanvas::Relayout() {
    const qreal dpr = Dpr();
    const int   w   = std::max(DeviceExtent(width(), dpr), 1);
    const int   h   = std::max(DeviceExtent(height(), dpr), 1);
    if (present_.width() != w || present_.height() != h) {
        present_ = QImage(w, h, QImage::Format_RGB32);
        present_.fill(Qt::black);
    }
    layout_.SetCanvasSize(w, h);
    UpdateScrollbars();
}

void QtPresenterCanvas::UpdateScrollbars() {
    const cerffrontend::ViewportLayout::Scroll s = layout_.UpdateScroll();
    const int            thick_v = vbar_->sizeHint().width();
    const int            thick_h = hbar_->sizeHint().height();
    const QSignalBlocker block_h(hbar_);
    const QSignalBlocker block_v(vbar_);
    hbar_->setVisible(s.horizontal);
    vbar_->setVisible(s.vertical);
    if (s.horizontal) {
        hbar_->setGeometry(0, height() - thick_h, width() - (s.vertical ? thick_v : 0), thick_h);
        hbar_->setRange(0, s.content_w - s.page_w);
        hbar_->setPageStep(s.page_w);
        hbar_->setValue(s.pos_x);
    }
    if (s.vertical) {
        vbar_->setGeometry(width() - thick_v, 0, thick_v, height() - (s.horizontal ? thick_h : 0));
        vbar_->setRange(0, s.content_h - s.page_h);
        vbar_->setPageStep(s.page_h);
        vbar_->setValue(s.pos_y);
    }
}

void QtPresenterCanvas::OnScroll(bool horizontal, int value) {
    layout_.ScrollTo(horizontal, value);
    QScrollBar*          bar = horizontal ? hbar_ : vbar_;
    const QSignalBlocker block(bar);
    bar->setValue(layout_.ScrollPos(horizontal));
    update();
}

void QtPresenterCanvas::ForceOpaque(uint32_t* pixels, size_t count) {
    for (size_t i = 0; i < count; ++i) pixels[i] |= 0xFF000000u;
}

void QtPresenterCanvas::VisibleSize(int& w, int& h) const {
    const qreal dpr = Dpr();
    w = DeviceExtent(width() - (vbar_->isVisible() ? vbar_->width() : 0), dpr);
    h = DeviceExtent(height() - (hbar_->isVisible() ? hbar_->height() : 0), dpr);
}
