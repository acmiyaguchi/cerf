#define NOMINMAX

#include "host_canvas_input.h"

#include <cerfcore/core/cerf_emulator.h>
#include "host_canvas.h"
#include "host_guest_cursor.h"

REGISTER_SERVICE(HostCanvasInput);

namespace {

uint32_t HeldButtons(WPARAM wp) {
    const WORD ks   = LOWORD(wp);
    uint32_t   held = 0;
    if (ks & MK_LBUTTON) held |= cerffrontend::kHeldLeft;
    if (ks & MK_RBUTTON) held |= cerffrontend::kHeldRight;
    if (ks & MK_MBUTTON) held |= cerffrontend::kHeldMiddle;
    return held;
}

bool ButtonMessage(UINT msg, cerffrontend::CanvasMouse& m) {
    using Kind = cerffrontend::CanvasMouse::Kind;
    using Btn  = cerffrontend::MouseButton;
    switch (msg) {
        case WM_LBUTTONDOWN: m.kind = Kind::kDown; m.button = Btn::kLeft;   return true;
        case WM_LBUTTONUP:   m.kind = Kind::kUp;   m.button = Btn::kLeft;   return true;
        case WM_RBUTTONDOWN: m.kind = Kind::kDown; m.button = Btn::kRight;  return true;
        case WM_RBUTTONUP:   m.kind = Kind::kUp;   m.button = Btn::kRight;  return true;
        case WM_MBUTTONDOWN: m.kind = Kind::kDown; m.button = Btn::kMiddle; return true;
        case WM_MBUTTONUP:   m.kind = Kind::kUp;   m.button = Btn::kMiddle; return true;
        case WM_MOUSEMOVE:   m.kind = Kind::kMove;                          return true;
    }
    return false;
}

}

void HostCanvasInput::OnReady() {
    emu_.Get<cerffrontend::CanvasInput>().Attach(this);
}

void HostCanvasInput::SetMouseCapture(bool on) {
    if (on) SetCapture(emu_.Get<HostCanvas>().Hwnd());
    else    ReleaseCapture();
}

bool HostCanvasInput::HasMouseCapture() const {
    return GetCapture() == emu_.Get<HostCanvas>().Hwnd();
}

void HostCanvasInput::FocusCanvas() {
    SetFocus(emu_.Get<HostCanvas>().Hwnd());
}

void HostCanvasInput::SetPointerHidden(bool hidden) {
    ShowCursor(hidden ? FALSE : TRUE);
}

void HostCanvasInput::WarpPointerTo(int x, int y) {
    POINT p = {x, y};
    ClientToScreen(emu_.Get<HostCanvas>().Hwnd(), &p);
    SetCursorPos(p.x, p.y);
}

void HostCanvasInput::ClientSize(int& width, int& height) const {
    RECT rc;
    GetClientRect(emu_.Get<HostCanvas>().Hwnd(), &rc);
    width  = rc.right - rc.left;
    height = rc.bottom - rc.top;
}

bool HostCanvasInput::HostToGuest(int x, int y, int& sx, int& sy) const {
    return emu_.Get<HostCanvas>().HostToGuest(x, y, sx, sy);
}

void HostCanvasInput::ClampGuest(int& sx, int& sy) const {
    emu_.Get<HostCanvas>().ClampGuest(sx, sy);
}

bool HostCanvasInput::SetCursorFor(HWND hwnd, LRESULT& out) {
    POINT p;
    GetCursorPos(&p);
    ScreenToClient(hwnd, &p);
    switch (emu_.Get<cerffrontend::CanvasInput>().CursorAt(p.x, p.y)) {
        case cerffrontend::CanvasCursor::kDefault:
            return false;
        case cerffrontend::CanvasCursor::kHand:
            SetCursor(LoadCursorW(nullptr, IDC_HAND));
            out = TRUE;
            return true;
        case cerffrontend::CanvasCursor::kNone:
            out = TRUE;
            return true;
        case cerffrontend::CanvasCursor::kGuest: {
            bool    active = false;
            HCURSOR cur    = emu_.Get<HostGuestCursor>().Resolve(active);
            if (!active) return false;
            SetCursor(cur);
            out = TRUE;
            return true;
        }
    }
    return false;
}

bool HostCanvasInput::Handle(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, LRESULT& out) {
    out         = 0;
    auto& input = emu_.Get<cerffrontend::CanvasInput>();
    input.RefreshLock();

    if (msg == WM_SETCURSOR) {
        if (LOWORD(lp) == HTCLIENT) return SetCursorFor(hwnd, out);
        return input.OnNonClientCursor(HeldButtons(wp));
    }

    if (msg == WM_KEYDOWN || msg == WM_KEYUP) {
        input.OnKey(static_cast<uint32_t>(wp), msg == WM_KEYUP);
        return true;
    }

    cerffrontend::CanvasMouse m;
    if (msg == WM_MOUSEWHEEL) {
        POINT p = {(int)(short)LOWORD(lp), (int)(short)HIWORD(lp)};
        ScreenToClient(hwnd, &p);
        m.kind  = cerffrontend::CanvasMouse::Kind::kWheel;
        m.x     = p.x;
        m.y     = p.y;
        m.held  = HeldButtons(wp);
        m.wheel = (int)(short)HIWORD(wp);
        return input.OnMouse(m);
    }
    if (msg == WM_CAPTURECHANGED) {
        m.kind = cerffrontend::CanvasMouse::Kind::kCaptureLost;
        return input.OnMouse(m);
    }
    if (!ButtonMessage(msg, m)) return false;
    m.x    = (int)(short)LOWORD(lp);
    m.y    = (int)(short)HIWORD(lp);
    m.held = HeldButtons(wp);
    return input.OnMouse(m);
}
