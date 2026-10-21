#include "qt_status_bar.h"

#include "qt_device_pixels.h"
#include "qt_menu_builder.h"
#include "qt_painter.h"
#include "qt_theme.h"

#include <cerffrontend/status_bar.h>

#include <cerfcore/host/host_widget.h>

#include <QEvent>
#include <QHelpEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

QtStatusBar::QtStatusBar(cerfcore::CerfEmulator& emu, QWidget* parent)
    : QWidget(parent), emu_(emu) {
    ApplyDeviceHeight();
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_AlwaysShowToolTips);
    QObject::connect(&tick_, &QTimer::timeout, this, [this] { Tick(); });
    tick_.start(static_cast<int>(cerffrontend::StatusBar::kTickMs));
}

void QtStatusBar::ApplyDeviceHeight() {
    setFixedHeight(LogicalExtentCovering(cerffrontend::StatusBar::kHeight, devicePixelRatioF()));
}

int QtStatusBar::DeviceWidth() const { return DeviceExtent(width(), devicePixelRatioF()); }

int QtStatusBar::DeviceHeight() const { return DeviceExtent(height(), devicePixelRatioF()); }

QRect QtStatusBar::BoxToLogical(int left, int top, int width, int height) const {
    return LogicalRect(left, top, width, height, devicePixelRatioF());
}

void QtStatusBar::paintEvent(QPaintEvent*) {
    const int w = DeviceWidth();
    const int h = DeviceHeight();
    if (w <= 0 || h <= 0) return;
    if (image_.width() != w || image_.height() != h) image_ = QImage(w, h, QImage::Format_RGB32);
    {
        QPainter  ip(&image_);
        QtPainter painter(emu_, ip);
        emu_.Get<cerffrontend::StatusBar>().Paint(painter, cerffrontend::Rect{0, 0, w, h},
                                                  emu_.Get<QtTheme>().IsDark());
    }
    const qreal dpr = devicePixelRatioF();
    QPainter    p(this);
    p.drawImage(QRectF(0, 0, w / dpr, h / dpr), image_);
}

void QtStatusBar::resizeEvent(QResizeEvent*) {
    if (emu_.Get<cerffrontend::StatusBar>().Relayout(DeviceWidth(), DeviceHeight())) update();
}

void QtStatusBar::changeEvent(QEvent* event) {
    if (event->type() == QEvent::DevicePixelRatioChange) {
        ApplyDeviceHeight();
        emu_.Get<cerffrontend::StatusBar>().Relayout(DeviceWidth(), DeviceHeight());
        update();
    }
    QWidget::changeEvent(event);
}

void QtStatusBar::Tick() {
    auto&                                     bar = emu_.Get<cerffrontend::StatusBar>();
    const cerffrontend::StatusBar::TickResult r   = bar.Tick(DeviceWidth(), DeviceHeight());
    if (tip_entry_ >= 0 && QToolTip::isVisible()) {
        for (size_t idx : r.tooltips_changed) {
            if (static_cast<int>(idx) != tip_entry_) continue;
            const cerffrontend::Rect& box = bar.Entries()[idx].box;
            QToolTip::showText(QCursor::pos(), QString::fromStdString(bar.TooltipAt(idx)), this,
                               BoxToLogical(box.left, box.top, box.Width(), box.Height()));
        }
    }
    if (r.layout_changed) tip_entry_ = -1;
    if (r.repaint) update();
}

int QtStatusBar::EntryAt(const QPoint& device_pos) const {
    const auto& entries = emu_.Get<cerffrontend::StatusBar>().Entries();
    for (size_t i = 0; i < entries.size(); ++i)
        if (entries[i].box.Contains(device_pos.x(), device_pos.y())) return static_cast<int>(i);
    return -1;
}

bool QtStatusBar::event(QEvent* event) {
    if (event->type() != QEvent::ToolTip) return QWidget::event(event);
    auto*     help  = static_cast<QHelpEvent*>(event);
    const int entry = EntryAt(DevicePoint(QPointF(help->pos()), devicePixelRatioF()));
    tip_entry_      = entry;
    if (entry < 0) {
        QToolTip::hideText();
        event->ignore();
        return true;
    }
    auto&                     bar = emu_.Get<cerffrontend::StatusBar>();
    const cerffrontend::Rect& box = bar.Entries()[static_cast<size_t>(entry)].box;
    QToolTip::showText(help->globalPos(), QString::fromStdString(bar.TooltipAt(entry)), this,
                       BoxToLogical(box.left, box.top, box.Width(), box.Height()));
    return true;
}

void QtStatusBar::SecondaryAction(cerfcore::HostWidget* w, const QPoint& pos) {
    std::vector<cerffrontend::MenuItem> items;
    if (!emu_.Get<cerffrontend::StatusBar>().ContextMenu(w, items)) return;
    emu_.Get<QtMenuBuilder>().RunContextMenu(items, mapToGlobal(pos));
}

void QtStatusBar::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) return;
    const QPoint device = DevicePoint(event->position(), devicePixelRatioF());
    auto&        bar    = emu_.Get<cerffrontend::StatusBar>();
    if (cerfcore::HostWidget* w = bar.WidgetAt(device.x()); w && !bar.PrimaryClick(w))
        SecondaryAction(w, event->position().toPoint());
}

void QtStatusBar::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() != Qt::RightButton) return;
    const QPoint device = DevicePoint(event->position(), devicePixelRatioF());
    if (cerfcore::HostWidget* w = emu_.Get<cerffrontend::StatusBar>().WidgetAt(device.x()))
        SecondaryAction(w, event->position().toPoint());
}

bool QtStatusBar::WidgetScreenRect(const cerfcore::HostWidget* w, QRect& out) const {
    cerffrontend::Rect box;
    if (!emu_.Get<cerffrontend::StatusBar>().WidgetBox(w, box)) return false;
    const QRect logical = BoxToLogical(box.left, box.top, box.Width(), box.Height());
    out = QRect(mapToGlobal(logical.topLeft()), logical.size());
    return true;
}

bool QtStatusBar::CaptureWidgetScreenRect(QRect& out) const {
    cerffrontend::Rect box;
    if (!emu_.Get<cerffrontend::StatusBar>().CaptureWidgetBox(box)) return false;
    const QRect logical = BoxToLogical(box.left, box.top, box.Width(), box.Height());
    out = QRect(mapToGlobal(logical.topLeft()), logical.size());
    return true;
}
