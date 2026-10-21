#pragma once

#include <cerffrontend/frontend_window.h>
#include "window_fullscreen.h"

#define NOMINMAX
#include <windows.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>

class HostWindow : public cerffrontend::FrontendWindow {
public:
    using cerffrontend::FrontendWindow::FrontendWindow;
    ~HostWindow() override;
    void OnReady() override;
    void OnShutdown() override;

    void OnLcdEnabled() override;

    void NotifyGuestRemoded(uint32_t guest_w, uint32_t guest_h) override;

    uint32_t GuestSurfaceWidth() const override;
    uint32_t GuestSurfaceHeight() const override;
    void     SetGuestSurfaceSize(uint32_t w, uint32_t h) override;

    Tab                        CurrentTab() const override;
    void                       SetTab(Tab tab, bool user_initiated) override;
    cerffrontend::ViewportMode Mode() const override;
    int                        IntegerFactor() const override;
    bool                       Antialias() const override;
    void                       SetViewportMode(cerffrontend::ViewportMode m) override;
    void                       SetIntegerScale(int factor) override;
    void                       SetAntialias(bool on) override;

    /* UI thread. Point the host framebuffer + canvas surface at a new guest
       mode. Shared by the guest re-mode ack (kGuestRemodeMsg) and the
       Change-resolution dialog's reset paths (where no guest ack arrives). */
    void SetGuestResolution(uint32_t w, uint32_t h) override;

    /* UI thread. Fit the window to a new sw x sh guest surface, unless the
       window is maximized - then keep it as-is and let the canvas viewport /
       scrollbars adapt. Used by the Change-resolution dialog. */
    void FitToResolution(uint32_t sw, uint32_t sh) override;

    /* Any thread. Run `job` on the main UI thread; dropped if the window is
       gone. Lets an action run off its caller's thread (a VGA card window
       ejecting itself must not run the eject on its own UI thread - the eject
       joins that thread). */
    void RunOnUiThread(std::function<void()> job) override;
    bool RunOnUiThreadAfter(uint32_t delay_ms, std::function<void()> job) override;

    bool     CaptureGuestSurface(std::vector<uint32_t>& out, uint32_t& w, uint32_t& h) override;
    uint64_t NativeHandle() const override { return reinterpret_cast<uintptr_t>(hwnd_); }

    HWND Hwnd() const { return hwnd_; }

    bool FollowGuest() const override { return follow_guest_; }
    void MatchGuestSize() override;
    void RefitIfFollowingGuest() override;

    bool IsFullscreen() const override { return fullscreen_.IsActive(); }
    void ToggleFullscreen() override;

    void BeginShutdownTeardown();

private:
    void StopUiThread();   /* idempotent: close window + join the UI thread */
    void UiThreadMain();
    static LRESULT CALLBACK WndProcStatic(HWND, UINT, WPARAM, LPARAM);
    LRESULT WndProc(HWND, UINT, WPARAM, LPARAM);

    void  AutoResizeToGuest();

    /* Shutdown sequence, all UI-thread, none run inside WM_CLOSE: a posted
       message shows the dialog, the save runs async, and teardown starts from
       the save-completion callback. */
    void  RunShutdownPrompt();

    void  FitWindowToSurface(uint32_t sw, uint32_t sh);

    std::thread             ui_thread_;
    std::atomic<bool>       ui_ready_{false};
    std::mutex              ui_ready_mutex_;
    std::condition_variable ui_ready_cv_;

    HWND  hwnd_ = nullptr;

    uint32_t initial_surface_w_ = 0;
    uint32_t initial_surface_h_ = 0;

    static constexpr UINT_PTR kDelayedJobFirst = 0x100;
    static constexpr UINT_PTR kDelayedJobLast  = 0x7FFF;

    std::map<UINT_PTR, std::function<void()>> delayed_jobs_;
    UINT_PTR                                  next_delayed_id_ = kDelayedJobFirst;

    bool follow_guest_  = true;   /* false once user resizes/maximizes */
    bool user_resizing_ = false;  /* between WM_ENTER/EXITSIZEMOVE */
    bool user_resized_  = false;

    WindowFullscreen fullscreen_;
};
