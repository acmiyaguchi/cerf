#define NOMINMAX

#include "host_status_bar.h"

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/log.h>
#include <cerfcore/host/host_widget.h>
#include <cerfcore/platform/wide_text_win32.h>
#include "host_dark_mode.h"
#include "win32_painter.h"

REGISTER_SERVICE(HostStatusBar);

namespace {
constexpr wchar_t kStatusClass[] = L"CerfHostStatusBar";
constexpr UINT    kTickTimerId   = 1;
}

void HostStatusBar::OnReady() {
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = &HostStatusBar::WndProcStatic;
    wc.hInstance     = GetModuleHandleW(nullptr);
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = kStatusClass;
    if (RegisterClassExW(&wc) == 0) {
        const DWORD err = GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS) {
            LOG(Caution, "HostStatusBar: RegisterClassExW failed (gle=%lu)\n", err);
            cerfcore::CerfFatalExit(cerfcore::CERF_FATAL_RUNTIME_ERROR);
        }
    }
}

void HostStatusBar::CreateOn(HWND parent, const RECT& rect) {
    hwnd_ = CreateWindowExW(0, kStatusClass, L"",
                            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                            rect.left, rect.top,
                            rect.right - rect.left, rect.bottom - rect.top,
                            parent, nullptr, GetModuleHandleW(nullptr), this);
    if (!hwnd_) {
        LOG(Caution, "HostStatusBar: CreateWindowExW failed (gle=%lu)\n",
            GetLastError());
        cerfcore::CerfFatalExit(cerfcore::CERF_FATAL_RUNTIME_ERROR);
    }

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_BAR_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);
    tip_hwnd_ = CreateWindowExW(0, TOOLTIPS_CLASSW, nullptr,
                                WS_POPUP | TTS_ALWAYSTIP, 0, 0, 0, 0,
                                hwnd_, nullptr, GetModuleHandleW(nullptr), nullptr);
    RebuildTooltips();
    SetTimer(hwnd_, kTickTimerId, cerffrontend::StatusBar::kTickMs, nullptr);
}

void HostStatusBar::Reposition(const RECT& r) {
    if (hwnd_) MoveWindow(hwnd_, r.left, r.top,
                          r.right - r.left, r.bottom - r.top, TRUE);
}

void HostStatusBar::RebuildTooltips() {
    if (!tip_hwnd_ || !hwnd_) return;

    for (size_t i = 1; i <= tip_count_; ++i) {
        TTTOOLINFOW ti = { TTTOOLINFOW_V1_SIZE };
        ti.hwnd = hwnd_;
        ti.uId  = (UINT_PTR)i;
        SendMessageW(tip_hwnd_, TTM_DELTOOLW, 0, (LPARAM)&ti);
    }

    const auto& entries = emu_.Get<cerffrontend::StatusBar>().Entries();
    tip_texts_.clear();
    tip_texts_.reserve(entries.size());
    for (const auto& e : entries)
        tip_texts_.push_back(cerfcore::Utf8ToWide(e.widget->Tooltip().c_str()));
    tip_count_ = entries.size();

    for (size_t i = 0; i < entries.size(); ++i) {
        TTTOOLINFOW ti = { TTTOOLINFOW_V1_SIZE };
        ti.uFlags   = 0;
        ti.hwnd     = hwnd_;
        ti.uId      = (UINT_PTR)(i + 1);
        ti.rect     = RECT{ entries[i].box.left, entries[i].box.top,
                            entries[i].box.right, entries[i].box.bottom };
        ti.lpszText = (LPWSTR)tip_texts_[i].c_str();
        SendMessageW(tip_hwnd_, TTM_ADDTOOLW, 0, (LPARAM)&ti);
    }
}

void HostStatusBar::UpdateTipText(size_t idx, std::wstring text) {
    if (!tip_hwnd_ || idx >= tip_texts_.size()) return;
    tip_texts_[idx] = std::move(text);
    TTTOOLINFOW ti = { TTTOOLINFOW_V1_SIZE };
    ti.hwnd     = hwnd_;
    ti.uId      = (UINT_PTR)(idx + 1);
    ti.lpszText = (LPWSTR)tip_texts_[idx].c_str();
    SendMessageW(tip_hwnd_, TTM_UPDATETIPTEXTW, 0, (LPARAM)&ti);
}

bool HostStatusBar::ToScreen(const cerffrontend::Rect& box, RECT& out) const {
    out = RECT{ box.left, box.top, box.right, box.bottom };
    MapWindowPoints(hwnd_, nullptr, reinterpret_cast<POINT*>(&out), 2);
    return true;
}

bool HostStatusBar::WidgetScreenRect(const cerfcore::HostWidget* w, RECT& out) const {
    cerffrontend::Rect box;
    return emu_.Get<cerffrontend::StatusBar>().WidgetBox(w, box) && ToScreen(box, out);
}

bool HostStatusBar::CaptureWidgetScreenRect(RECT& out) const {
    cerffrontend::Rect box;
    return emu_.Get<cerffrontend::StatusBar>().CaptureWidgetBox(box) && ToScreen(box, out);
}

void HostStatusBar::SecondaryAction(cerfcore::HostWidget* w, int x, int y, UINT button_flag) {
    std::vector<cerffrontend::MenuItem> items;
    if (!emu_.Get<cerffrontend::StatusBar>().ContextMenu(w, items)) return;
    HMENU m = binder_.BuildContextMenu(items);
    POINT pt = { x, y };
    ClientToScreen(hwnd_, &pt);
    const int id = (int)TrackPopupMenu(m, TPM_RETURNCMD | TPM_LEFTALIGN | button_flag,
                                       pt.x, pt.y, 0, hwnd_, nullptr);
    DestroyMenu(m);
    if (id) binder_.Dispatch(id);
}

LRESULT CALLBACK HostStatusBar::WndProcStatic(HWND hwnd, UINT msg,
                                              WPARAM wp, LPARAM lp) {
    HostStatusBar* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = static_cast<HostStatusBar*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<HostStatusBar*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }
    return self ? self->WndProc(hwnd, msg, wp, lp)
                : DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT HostStatusBar::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (tip_hwnd_ && (msg == WM_MOUSEMOVE || msg == WM_LBUTTONDOWN ||
                      msg == WM_LBUTTONUP || msg == WM_RBUTTONDOWN ||
                      msg == WM_RBUTTONUP)) {
        MSG mm = {};
        mm.hwnd    = hwnd;
        mm.message = msg;
        mm.wParam  = wp;
        mm.lParam  = lp;
        SendMessageW(tip_hwnd_, TTM_RELAYEVENT, (WPARAM)GetMessagePos(), (LPARAM)&mm);
    }

    auto& bar = emu_.Get<cerffrontend::StatusBar>();
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);

            HDC     mem    = CreateCompatibleDC(dc);
            HBITMAP bmp    = CreateCompatibleBitmap(dc, rc.right, rc.bottom);
            HGDIOBJ oldbmp = SelectObject(mem, bmp);

            Win32Painter painter(emu_, mem);
            bar.Paint(painter, cerffrontend::Rect{ rc.left, rc.top, rc.right, rc.bottom },
                      emu_.Get<HostDarkMode>().IsDark());

            BitBlt(dc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);

            SelectObject(mem, oldbmp);
            DeleteObject(bmp);
            DeleteDC(mem);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_SIZE: {
            RECT rc;
            GetClientRect(hwnd, &rc);
            if (bar.Relayout(rc.right, rc.bottom)) RebuildTooltips();
            return 0;
        }

        case WM_TIMER:
            if (wp == kTickTimerId) {
                RECT rc;
                GetClientRect(hwnd, &rc);
                const cerffrontend::StatusBar::TickResult r = bar.Tick(rc.right, rc.bottom);
                if (r.layout_changed) RebuildTooltips();
                for (size_t idx : r.tooltips_changed)
                    UpdateTipText(idx, cerfcore::Utf8ToWide(bar.TooltipAt(idx).c_str()));
                if (r.repaint) InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;

        case WM_LBUTTONDOWN: {
            const int x = (int)(short)LOWORD(lp);
            cerfcore::HostWidget* w = bar.WidgetAt(x);
            if (w && !bar.PrimaryClick(w))
                SecondaryAction(w, x, (int)(short)HIWORD(lp), TPM_LEFTBUTTON);
            return 0;
        }

        case WM_RBUTTONUP: {
            const int x = (int)(short)LOWORD(lp);
            if (cerfcore::HostWidget* w = bar.WidgetAt(x))
                SecondaryAction(w, x, (int)(short)HIWORD(lp), TPM_RIGHTBUTTON);
            return 0;
        }

        case WM_DESTROY:
            KillTimer(hwnd, kTickTimerId);
            if (tip_hwnd_) { DestroyWindow(tip_hwnd_); tip_hwnd_ = nullptr; }
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
