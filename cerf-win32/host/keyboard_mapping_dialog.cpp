#define NOMINMAX
#include "keyboard_mapping_dialog.h"

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/host/keyboard_map.h>
#include <cerfcore/platform/wide_text_win32.h>
#include <cerffrontend/keyboard_mapping_panel.h>
#include "host_dark_mode.h"
#include "host_window.h"
#include "win32_painter.h"

REGISTER_SERVICE_AS(KeyboardMappingDialog, cerfcore::KeyboardMappingView);

namespace {

using Panel = cerffrontend::KeyboardMappingPanel;

constexpr wchar_t kClass[] = L"CerfKeyboardMappingDlg";

}

bool KeyboardMappingDialog::ShouldRegister() {
    return emu_.TryGet<cerfcore::KeyboardMap>() != nullptr;
}

void KeyboardMappingDialog::OnReady() {
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = &KeyboardMappingDialog::WndProcStatic;
    wc.hInstance     = GetModuleHandleW(nullptr);
    wc.hIcon         = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(1));
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = kClass;
    RegisterClassExW(&wc);
}

void KeyboardMappingDialog::Show() {
    if (hwnd_) { SetForegroundWindow(hwnd_); return; }

    HWND owner = emu_.Get<HostWindow>().Hwnd();
    emu_.Get<Panel>().Reset();

    const DWORD style = WS_CAPTION | WS_SYSMENU | WS_DLGFRAME | WS_POPUP;
    RECT wr = { 0, 0, Panel::kClientW, Panel::kClientH };
    AdjustWindowRect(&wr, style, FALSE);
    const int ww = wr.right - wr.left;
    const int wh = wr.bottom - wr.top;
    RECT orc = { 0, 0, 0, 0 };
    GetWindowRect(owner, &orc);
    const int wx = orc.left + ((orc.right - orc.left) - ww) / 2;
    const int wy = orc.top  + ((orc.bottom - orc.top) - wh) / 2;

    hwnd_ = CreateWindowExW(WS_EX_TOOLWINDOW, kClass, cerfcore::Utf8ToWide(Panel::kTitle).c_str(),
                            style, wx, wy, ww, wh, owner, nullptr,
                            GetModuleHandleW(nullptr), this);
    if (!hwnd_) return;

    emu_.Get<HostDarkMode>().ApplyToDialog(hwnd_);
    ShowWindow(hwnd_, SW_SHOW);
    SetForegroundWindow(hwnd_);
}

LRESULT KeyboardMappingDialog::WndProc(HWND hwnd, UINT msg, WPARAM wp,
                                       LPARAM lp) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(hwnd, &ps);
            RECT client;
            GetClientRect(hwnd, &client);
            Win32Painter painter(emu_, dc);
            emu_.Get<Panel>().Paint(painter,
                                    cerffrontend::Rect{client.left, client.top, client.right,
                                                       client.bottom},
                                    emu_.Get<HostDarkMode>().IsDark());
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_LBUTTONUP:
            if (emu_.Get<Panel>().OnClick((int)(short)LOWORD(lp), (int)(short)HIWORD(lp)))
                InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_KEYDOWN:
            if (wp == VK_ESCAPE) DestroyWindow(hwnd);
            return 0;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            hwnd_ = nullptr;
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT CALLBACK KeyboardMappingDialog::WndProcStatic(HWND hwnd, UINT msg,
                                                      WPARAM wp, LPARAM lp) {
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
    }
    auto* self = reinterpret_cast<KeyboardMappingDialog*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self) return self->WndProc(hwnd, msg, wp, lp);
    return DefWindowProcW(hwnd, msg, wp, lp);
}
