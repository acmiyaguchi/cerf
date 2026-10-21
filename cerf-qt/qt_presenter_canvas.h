#pragma once

#include <cerffrontend/viewport_layout.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/host/frame_source.h>

#include <QImage>
#include <QTimer>
#include <QWidget>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

class QScrollBar;

class QtPresenterCanvas : public QWidget {
public:
    QtPresenterCanvas(cerfcore::CerfEmulator& emu, QWidget* parent, cerfcore::FrameSource& source,
                      uint32_t surf_w, uint32_t surf_h, int present_interval_ms);

    uint32_t GuestSurfaceWidth() const { return layout_.SurfaceWidth(); }
    uint32_t GuestSurfaceHeight() const { return layout_.SurfaceHeight(); }
    int      ContentWidth() const { return layout_.ContentWidth(); }
    int      ContentHeight() const { return layout_.ContentHeight(); }
    void     SetGuestSurfaceSize(uint32_t w, uint32_t h);

    cerffrontend::ViewportMode Mode() const { return layout_.Mode(); }
    int                        IntegerFactor() const { return layout_.IntegerFactor(); }
    bool                       Antialias() const { return layout_.Antialias(); }
    void                       SetViewportMode(cerffrontend::ViewportMode m);
    void                       SetIntegerScale(int factor);
    void                       SetAntialias(bool on);
    void                       SetFramebufferActive(bool active);

    bool CaptureSurface(std::vector<uint32_t>& out, uint32_t& w, uint32_t& h);
    void SetResizeListener(std::function<void()> listener) { on_resized_ = std::move(listener); }

    qreal Dpr() const { return devicePixelRatioF(); }
    void  VisibleSize(int& w, int& h) const;
    bool  MapToGuest(int x, int y, int& sx, int& sy) const { return layout_.HostToGuest(x, y, sx, sy); }
    void  ClampToGuest(int& sx, int& sy) const { layout_.ClampGuest(sx, sy); }

protected:
    virtual void OnPresentTick() {}
    virtual bool RenderAlt(uint32_t*, int, int) { return false; }
    virtual bool ShouldDesaturate() { return false; }
    virtual void RenderOverlay(QImage&, int, int) {}

    bool SourceHasFrame() const { return source_.HasFrame(); }

    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void changeEvent(QEvent* event) override;

    cerfcore::CerfEmulator& emu_;

private:
    void TickAndPresent();
    bool ComposeInto();
    void Relayout();
    void UpdateScrollbars();
    void OnScroll(bool horizontal, int value);

    static void ForceOpaque(uint32_t* pixels, size_t count);

    cerfcore::FrameSource&       source_;
    cerffrontend::ViewportLayout layout_;
    QImage                       guest_;
    QImage                       present_;
    QScrollBar*                  hbar_ = nullptr;
    QScrollBar*                  vbar_ = nullptr;
    QTimer                       present_timer_;
    std::function<void()>        on_resized_;
};
