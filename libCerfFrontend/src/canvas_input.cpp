#include <cerffrontend/canvas_input.h>

#include <cerffrontend/balloon_hint.h>
#include <cerffrontend/boot_screen.h>
#include <cerffrontend/canvas_tabs.h>
#include <cerffrontend/frontend_window.h>
#include <cerffrontend/host_key_binding.h>
#include <cerffrontend/input_capture.h>
#include <cerffrontend/notification_overlay.h>
#include <cerffrontend/pointer_stylus_simulation.h>
#include <cerffrontend/stylus_alt_tap.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/fatal.h>
#include <cerfcore/host/keyboard_router.h>
#include <cerfcore/host/notification_stack.h>
#include <cerfcore/host/pointer_input.h>
#include <cerfcore/host/pointer_router.h>
#include <cerfcore/host/pointer_source.h>
#include <cerfcore/host/relative_mouse_input.h>
#include <cerfcore/host/touch_input.h>

namespace cerffrontend {

REGISTER_SERVICE(CanvasInput);

namespace {

constexpr uint32_t kLockHintHoldMs = 4500;

using Kind = CanvasMouse::Kind;
using Tab  = cerfcore::CanvasTab;

uint32_t ButtonBit(MouseButton b) {
    switch (b) {
        case MouseButton::kLeft:   return kHeldLeft;
        case MouseButton::kRight:  return kHeldRight;
        case MouseButton::kMiddle: return kHeldMiddle;
        case MouseButton::kNone:   return 0;
    }
    return 0;
}

}

CanvasInputHost& CanvasInput::Host() {
    if (!host_) emu_.Get<cerfcore::Fatal>().Die("CanvasInput used with no canvas attached");
    return *host_;
}

bool CanvasInput::Locked(bool framebuffer) const {
    cerfcore::PointerSource* active = emu_.Get<cerfcore::PointerRouter>().Active();
    auto*                    cap    = emu_.TryGet<InputCapture>();
    return active && active->Kind() == cerfcore::PointerKind::Relative && cap &&
           cap->IsCaptured() && framebuffer;
}

void CanvasInput::EnsureLocked() {
    if (mouse_locked_active_) return;
    mouse_locked_active_ = true;
    CanvasInputHost& h   = Host();
    h.FocusCanvas();
    h.SetMouseCapture(true);
    h.SetPointerHidden(true);
    int w = 0, ht = 0;
    h.ClientSize(w, ht);
    h.WarpPointerTo(w / 2, ht / 2);
}

void CanvasInput::Unlock() {
    if (!mouse_locked_active_) return;
    mouse_locked_active_ = false;
    CanvasInputHost& h   = Host();
    if (h.HasMouseCapture()) h.SetMouseCapture(false);
    h.SetPointerHidden(false);
}

void CanvasInput::ShowLockHintOnce() {
    if (lock_hint_shown_) return;
    lock_hint_shown_ = true;
    emu_.Get<BalloonHint>().ShowUnderCaptureWidget(
        "Mouse locked - press " + emu_.Get<HostKeyBinding>().Label() + " to release",
        kLockHintHoldMs);
}

void CanvasInput::ReleasePenIfDown() {
    emu_.Get<StylusAltTap>().Cancel();
    if (!pen_down_) return;
    pen_down_ = false;
    CanvasInputHost& h = Host();
    if (h.HasMouseCapture()) h.SetMouseCapture(false);
    if (auto* t = emu_.TryGet<cerfcore::TouchInput>()) t->OnCaptureLost();
}

bool CanvasInput::RouteNotification(const CanvasMouse& m) {
    if (m.kind != Kind::kDown && m.kind != Kind::kUp) return false;
    const uint32_t bit = ButtonBit(m.button);
    if (bit == 0) return false;
    if (m.kind == Kind::kUp) {
        if (!(notification_click_mask_ & bit)) return false;
        notification_click_mask_ &= ~bit;
        return true;
    }
    uint32_t id = 0;
    if (!emu_.Get<NotificationOverlay>().HitTest(m.x, m.y, id)) return false;
    notification_click_mask_ |= bit;
    if (m.button == MouseButton::kLeft) emu_.Get<cerfcore::NotificationStack>().Dismiss(id);
    return true;
}

uint32_t CanvasInput::RelativeMask(uint32_t held) {
    uint32_t mask = 0;
    if (held & kHeldLeft) mask |= cerfcore::kRelMouseLeft;
    if (held & kHeldRight) mask |= cerfcore::kRelMouseRight;
    return mask;
}

bool CanvasInput::OnNonClientCursor(uint32_t held) {
    if (!Locked(emu_.Get<CanvasTabs>().Current() == Tab::Framebuffer)) return false;
    auto* rm = emu_.TryGet<cerfcore::RelativeMouseInput>();
    if (!rm) return false;
    EnsureLocked();
    rm->OnRelativeMove(0, 0, RelativeMask(held));
    return true;
}

bool CanvasInput::RouteLocked(const CanvasMouse& m) {
    const bool lr = (m.kind == Kind::kDown || m.kind == Kind::kUp) &&
                    (m.button == MouseButton::kLeft || m.button == MouseButton::kRight);
    if (m.kind != Kind::kMove && !lr) return false;
    auto* rm = emu_.TryGet<cerfcore::RelativeMouseInput>();
    if (!rm) return false;
    EnsureLocked();

    const uint32_t mask = RelativeMask(m.held);

    if (m.kind == Kind::kMove) {
        int w = 0, h = 0;
        Host().ClientSize(w, h);
        const int dx = m.x - w / 2;
        const int dy = m.y - h / 2;
        if (dx == 0 && dy == 0) return true;
        rm->OnRelativeMove(dx, dy, mask);
        Host().WarpPointerTo(w / 2, h / 2);
        return true;
    }
    rm->OnRelativeMove(0, 0, mask);
    return true;
}

bool CanvasInput::RoutePointer(const CanvasMouse& m, bool framebuffer) {
    auto* pi = emu_.TryGet<cerfcore::PointerInput>();
    if (!pi || !framebuffer) return false;
    CanvasInputHost& h = Host();

    if (m.kind == Kind::kWheel) {
        int sx = 0, sy = 0;
        h.HostToGuest(m.x, m.y, sx, sy);
        h.ClampGuest(sx, sy);
        pi->OnWheel(sx, sy, m.wheel);
        return true;
    }
    if (m.kind == Kind::kCaptureLost) {
        pi->OnCaptureLost();
        return true;
    }

    const bool down = m.kind == Kind::kDown && m.button != MouseButton::kNone;
    const bool up   = m.kind == Kind::kUp && m.button != MouseButton::kNone;
    if (!down && !up && m.kind != Kind::kMove) return false;

    if (down) {
        h.FocusCanvas();
        h.SetMouseCapture(true);
    }

    int sx = 0, sy = 0;
    h.HostToGuest(m.x, m.y, sx, sy);
    h.ClampGuest(sx, sy);

    uint32_t mask = 0;
    if (m.held & kHeldLeft) mask |= cerfcore::kPointerLeft;
    if (m.held & kHeldRight) mask |= cerfcore::kPointerRight;
    if (m.held & kHeldMiddle) mask |= cerfcore::kPointerMiddle;

    const bool held   = (mask & (cerfcore::kPointerLeft | cerfcore::kPointerRight)) != 0;
    const bool report = down || up || held || !emu_.Get<PointerStylusSimulation>().Enabled();
    if (report) pi->OnMove(sx, sy, mask);

    if (up && mask == 0) h.SetMouseCapture(false);
    return true;
}

bool CanvasInput::RouteStylus(const CanvasMouse& m, bool framebuffer) {
    CanvasInputHost& h     = Host();
    auto*            touch = emu_.TryGet<cerfcore::TouchInput>();
    int              sx = 0, sy = 0;

    if (m.kind == Kind::kDown && m.button == MouseButton::kRight) {
        h.FocusCanvas();
        if (!framebuffer || pen_down_) return true;
        if (!h.HostToGuest(m.x, m.y, sx, sy)) return true;
        emu_.Get<StylusAltTap>().Begin(sx, sy);
        return true;
    }
    if (m.kind == Kind::kUp && m.button == MouseButton::kRight) return true;
    if (m.kind == Kind::kDown && m.button == MouseButton::kLeft) {
        h.FocusCanvas();
        if (!framebuffer || emu_.Get<StylusAltTap>().Running()) return true;
        if (!h.HostToGuest(m.x, m.y, sx, sy)) return true;
        pen_down_ = true;
        h.SetMouseCapture(true);
        if (touch) touch->OnPenDown(sx, sy);
        return true;
    }
    if (m.kind == Kind::kMove) {
        if (!pen_down_) return true;
        h.HostToGuest(m.x, m.y, sx, sy);
        h.ClampGuest(sx, sy);
        if (touch) touch->OnPenMove(sx, sy);
        return true;
    }
    if (m.kind == Kind::kUp && m.button == MouseButton::kLeft) {
        if (!pen_down_) return true;
        pen_down_ = false;
        h.SetMouseCapture(false);
        h.HostToGuest(m.x, m.y, sx, sy);
        h.ClampGuest(sx, sy);
        if (touch) touch->OnPenUp(sx, sy);
        return true;
    }
    if (m.kind == Kind::kCaptureLost) {
        if (pen_down_) {
            pen_down_ = false;
            if (touch) touch->OnCaptureLost();
        }
        return true;
    }
    return false;
}

bool CanvasInput::OnMouse(const CanvasMouse& m) {
    const Tab  tab         = emu_.Get<CanvasTabs>().Current();
    const bool framebuffer = tab == Tab::Framebuffer;
    cerfcore::PointerKind kind = cerfcore::PointerKind::Absolute;
    if (auto* a = emu_.Get<cerfcore::PointerRouter>().Active()) kind = a->Kind();
    auto*      cap    = emu_.TryGet<InputCapture>();
    const bool locked = Locked(framebuffer);

    if (!locked && RouteNotification(m)) return true;

    if (tab == Tab::Boot && m.kind == Kind::kDown && m.button == MouseButton::kLeft &&
        emu_.Get<BootScreen>().HitTestHwLine(m.x, m.y)) {
        emu_.Get<FrontendWindow>().SetTab(Tab::Hw, true);
        return true;
    }

    if (locked) {
        if (RouteLocked(m)) return true;
    } else {
        Unlock();
        const bool click = m.kind == Kind::kDown &&
                           (m.button == MouseButton::kLeft || m.button == MouseButton::kRight);
        if (kind == cerfcore::PointerKind::Relative && click && cap && framebuffer) {
            cap->SetCaptured(true);
            ShowLockHintOnce();
            return true;
        }
    }

    if (kind == cerfcore::PointerKind::Absolute && RoutePointer(m, framebuffer)) return true;
    if (kind == cerfcore::PointerKind::Stylus) return RouteStylus(m, framebuffer);
    return false;
}

CanvasCursor CanvasInput::CursorAt(int x, int y) {
    const Tab  tab         = emu_.Get<CanvasTabs>().Current();
    const bool framebuffer = tab == Tab::Framebuffer;
    cerfcore::PointerKind kind = cerfcore::PointerKind::Absolute;
    if (auto* a = emu_.Get<cerfcore::PointerRouter>().Active()) kind = a->Kind();
    const bool locked = Locked(framebuffer);

    uint32_t id = 0;
    if (!locked && emu_.Get<NotificationOverlay>().HitTest(x, y, id)) return CanvasCursor::kHand;
    if (tab == Tab::Boot && emu_.Get<BootScreen>().HitTestHwLine(x, y)) return CanvasCursor::kHand;

    if (locked) {
        if (emu_.TryGet<cerfcore::RelativeMouseInput>()) {
            EnsureLocked();
            return CanvasCursor::kNone;
        }
    } else {
        Unlock();
    }
    if (framebuffer && kind == cerfcore::PointerKind::Absolute) return CanvasCursor::kGuest;
    return CanvasCursor::kDefault;
}

void CanvasInput::RefreshLock() {
    if (!Locked(emu_.Get<CanvasTabs>().Current() == Tab::Framebuffer)) Unlock();
}

void CanvasInput::OnKey(uint32_t vk, bool key_up) {
    RefreshLock();
    if (emu_.Get<CanvasTabs>().Current() == Tab::Framebuffer)
        emu_.Get<cerfcore::KeyboardRouter>().OnHostKey(static_cast<uint8_t>(vk), key_up);
}

}
