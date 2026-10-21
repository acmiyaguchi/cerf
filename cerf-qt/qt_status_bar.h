#pragma once

#include <cerfcore/core/cerf_emulator.h>

#include <QImage>
#include <QTimer>
#include <QWidget>

namespace cerfcore { class HostWidget; }

class QtStatusBar : public QWidget {
public:
    QtStatusBar(cerfcore::CerfEmulator& emu, QWidget* parent);

    bool WidgetScreenRect(const cerfcore::HostWidget* w, QRect& out) const;
    bool CaptureWidgetScreenRect(QRect& out) const;

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void changeEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    bool event(QEvent* event) override;

private:
    void  Tick();
    void  ApplyDeviceHeight();
    int   DeviceWidth() const;
    int   DeviceHeight() const;
    QRect BoxToLogical(int left, int top, int width, int height) const;
    void  SecondaryAction(cerfcore::HostWidget* w, const QPoint& pos);
    int   EntryAt(const QPoint& device_pos) const;

    cerfcore::CerfEmulator& emu_;
    QTimer                  tick_;
    QImage                  image_;
    int                     tip_entry_ = -1;
};
