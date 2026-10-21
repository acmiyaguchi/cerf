#define NOMINMAX

#include "host_input_capture.h"

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/log.h>

REGISTER_SERVICE_AS(HostInputCapture, cerffrontend::InputCapture);

namespace {

thread_local HostInputCapture* t_active = nullptr;

LRESULT CALLBACK LlKeyboardProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION && t_active) {
        auto* k = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
        if (t_active->OnHookKey(wParam, k)) return 1;
    }
    return CallNextHookEx(nullptr, code, wParam, lParam);
}

}

HostInputCapture::~HostInputCapture() { DetachUiThread(); }

void HostInputCapture::AttachUiThread(HWND host_hwnd) {
    host_hwnd_ = host_hwnd;
    t_active = this;
    hook_ = SetWindowsHookExW(WH_KEYBOARD_LL, &LlKeyboardProc,
                              GetModuleHandleW(nullptr), 0);
    if (!hook_)
        LOG(Caution, "HostInputCapture: SetWindowsHookEx failed (gle=%lu)\n",
            GetLastError());
}

void HostInputCapture::DetachUiThread() {
    if (hook_) { UnhookWindowsHookEx(hook_); hook_ = nullptr; }
    if (t_active == this) t_active = nullptr;
}

bool HostInputCapture::IsForeground() const {
    return host_hwnd_ && GetForegroundWindow() == host_hwnd_;
}

bool HostInputCapture::OnHookKey(WPARAM wParam, const KBDLLHOOKSTRUCT* k) {
    if (!IsForeground()) return false;
    return OnHostKey(k->vkCode, wParam == WM_KEYUP || wParam == WM_SYSKEYUP);
}
