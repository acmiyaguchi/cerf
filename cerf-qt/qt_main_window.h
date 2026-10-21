#pragma once

#include <cerffrontend/frontend_window.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

class QTimer;
class QWidget;
class QtCanvas;
class QtHostWindow;
class QtStatusBar;

class QtMainWindow : public cerffrontend::FrontendWindow {
public:
    using cerffrontend::FrontendWindow::FrontendWindow;
    ~QtMainWindow() override;

    void OnReady() override;
    void OnShutdown() override;

    Tab                        CurrentTab() const override;
    void                       SetTab(Tab tab, bool user_initiated) override;
    cerffrontend::ViewportMode Mode() const override;
    int                        IntegerFactor() const override;
    bool                       Antialias() const override;
    void                       SetViewportMode(cerffrontend::ViewportMode m) override;
    void                       SetIntegerScale(int factor) override;
    void                       SetAntialias(bool on) override;

    bool IsFullscreen() const override { return fullscreen_; }
    void ToggleFullscreen() override;
    bool FollowGuest() const override { return follow_guest_; }
    void MatchGuestSize() override;
    void RefitIfFollowingGuest() override;

    void SetGuestResolution(uint32_t w, uint32_t h) override;
    void FitToResolution(uint32_t w, uint32_t h) override;

    void     OnLcdEnabled() override;
    void     NotifyGuestRemoded(uint32_t w, uint32_t h) override;
    uint32_t GuestSurfaceWidth() const override { return surface_w_.load(std::memory_order_acquire); }
    uint32_t GuestSurfaceHeight() const override { return surface_h_.load(std::memory_order_acquire); }
    void     SetGuestSurfaceSize(uint32_t w, uint32_t h) override;

    void     RunOnUiThread(std::function<void()> job) override;
    bool     RunOnUiThreadAfter(uint32_t delay_ms, std::function<void()> job) override;
    bool     CaptureGuestSurface(std::vector<uint32_t>& out, uint32_t& w, uint32_t& h) override;
    uint64_t NativeHandle() const override { return native_handle_.load(std::memory_order_acquire); }
    void     Raise();

    QWidget*     Window() const;
    QWidget*     CanvasWidget() const;
    QtStatusBar& Bar() const { return *bar_; }

    void SetKeyboardGrab(bool on);
    void CloseRequested();
    void BeginShutdownTeardown();

    void OnUserResized() { follow_guest_ = false; }
    void OnMaximized() { follow_guest_ = false; }
    void OnDeactivated();
    void OnCanvasResized();

private:
    void BuildMenus();
    void AutoResizeToGuest();
    void FitWindowToSurface(uint32_t sw, uint32_t sh);
    void ChromeExtent(int& extra_w, int& extra_h) const;
    void AdoptResolutionToWindowMonitor();
    void RunShutdownPrompt();
    void PollTeardown();
    void FireAutoResize();
    bool OnUiThread() const;

    QtHostWindow*           window_ = nullptr;
    QtCanvas*               canvas_ = nullptr;
    QtStatusBar*            bar_    = nullptr;
    std::unique_ptr<QTimer> teardown_timer_;
    std::unique_ptr<QTimer> resize_debounce_;

    std::atomic<uint32_t> surface_w_{0};
    std::atomic<uint32_t> surface_h_{0};
    std::atomic<uint64_t> native_handle_{0};

    uint32_t initial_surface_w_ = 0;
    uint32_t initial_surface_h_ = 0;
    bool     follow_guest_      = true;
    bool     fullscreen_        = false;
    bool     was_maximized_     = false;
};
