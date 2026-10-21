#define NOMINMAX

#include <cerffrontend/balloon_hint.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/platform/wide_text_win32.h>
#include "host_canvas.h"
#include "host_status_bar.h"

#include <windows.h>
#include <commctrl.h>

#include <string>

namespace {

constexpr wchar_t  kClass[]       = L"CerfBalloonHintHost";
constexpr UINT_PTR kDismissTimer  = 0xC1;

class HostBalloonHint : public cerffrontend::BalloonHint {
public:
    using cerffrontend::BalloonHint::BalloonHint;

    void OnReady() override;
    void OnShutdown() override;

    void ShowUnderWidget(const cerfcore::HostWidget* anchor, const std::string& text,
                         uint32_t hold_ms) override;
    void ShowUnderCaptureWidget(const std::string& text, uint32_t hold_ms) override;

private:
    static LRESULT CALLBACK WndProcStatic(HWND, UINT, WPARAM, LPARAM);

    void Show(const RECT* anchor, const std::string& text, UINT hold_ms);
    void Dismiss();

    HWND host_ = nullptr;
    HWND tip_  = nullptr;
    std::wstring shown_text_;
};

void HostBalloonHint::OnReady() {
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = &HostBalloonHint::WndProcStatic;
    wc.hInstance     = GetModuleHandleW(nullptr);
    wc.lpszClassName = kClass;
    RegisterClassExW(&wc);
}

void HostBalloonHint::OnShutdown() {
    if (tip_)  { DestroyWindow(tip_);  tip_  = nullptr; }
    if (host_) { DestroyWindow(host_); host_ = nullptr; }
}

LRESULT CALLBACK HostBalloonHint::WndProcStatic(HWND hwnd, UINT msg, WPARAM wp,
                                                LPARAM lp) {
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
    }
    auto* self =
        reinterpret_cast<HostBalloonHint*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self && msg == WM_TIMER && wp == kDismissTimer) {
        KillTimer(hwnd, kDismissTimer);
        self->Dismiss();
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void HostBalloonHint::Dismiss() {
    if (!tip_) return;
    DestroyWindow(tip_);
    tip_ = nullptr;
}

void HostBalloonHint::ShowUnderWidget(const cerfcore::HostWidget* anchor,
                                      const std::string& text, uint32_t hold_ms) {
    RECT rc;
    if (emu_.Get<HostStatusBar>().WidgetScreenRect(anchor, rc))
        Show(&rc, text, hold_ms);
    else
        Show(nullptr, text, hold_ms);
}

void HostBalloonHint::ShowUnderCaptureWidget(const std::string& text, uint32_t hold_ms) {
    RECT rc;
    if (emu_.Get<HostStatusBar>().CaptureWidgetScreenRect(rc))
        Show(&rc, text, hold_ms);
    else
        Show(nullptr, text, hold_ms);
}

void HostBalloonHint::Show(const RECT* anchor, const std::string& text,
                           UINT hold_ms) {
    HWND canvas = emu_.Get<HostCanvas>().Hwnd();
    if (!canvas) return;

    Dismiss();
    if (!host_) {
        host_ = CreateWindowExW(0, kClass, nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE,
                                nullptr, GetModuleHandleW(nullptr), this);
        if (!host_) return;
    }

    tip_ = CreateWindowExW(
        WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr,
        WS_POPUP | TTS_NOPREFIX | TTS_BALLOON | TTS_ALWAYSTIP,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        canvas, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!tip_) return;

    TTTOOLINFOW ti = { TTTOOLINFOW_V1_SIZE };
    ti.uFlags   = TTF_TRACK | TTF_IDISHWND;
    ti.hwnd     = canvas;
    ti.uId      = reinterpret_cast<UINT_PTR>(canvas);
    shown_text_ = cerfcore::Utf8ToWide(text.c_str());
    ti.lpszText = const_cast<wchar_t*>(shown_text_.c_str());
    SendMessageW(tip_, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&ti));

    POINT p;
    if (anchor) {
        p.x = (anchor->left + anchor->right) / 2;
        p.y = anchor->bottom;
    } else {
        RECT rc;
        GetClientRect(canvas, &rc);
        p = { (rc.right - rc.left) / 2, rc.bottom - kCanvasBottomGap };
        ClientToScreen(canvas, &p);
    }
    SendMessageW(tip_, TTM_TRACKPOSITION, 0, MAKELPARAM(p.x, p.y));
    SendMessageW(tip_, TTM_TRACKACTIVATE, TRUE, reinterpret_cast<LPARAM>(&ti));
    SetTimer(host_, kDismissTimer, hold_ms, nullptr);
}

}

REGISTER_SERVICE_AS(HostBalloonHint, cerffrontend::BalloonHint);
