#include "host_focus_policy.h"

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/device_config.h>
#include <cerfcore/core/log.h>

REGISTER_SERVICE(HostFocusPolicy);

bool HostFocusPolicy::MayActivate() const {
    if (!emu_.Get<cerfcore::DeviceConfig>().no_focus) return true;
    DWORD pid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    return pid == GetCurrentProcessId();
}

void HostFocusPolicy::Show(HWND hwnd) const {
    if (MayActivate()) {
        ShowWindow(hwnd, SW_SHOW);
        return;
    }
    LOG(Lcd, "HostFocusPolicy: showing %p at the Z-order bottom, not activated\n",
        (void*)hwnd);
    SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void HostFocusPolicy::Raise(HWND hwnd) const {
    if (MayActivate()) SetForegroundWindow(hwnd);
}

void HostFocusPolicy::Focus(HWND hwnd) const {
    if (MayActivate()) SetFocus(hwnd);
}
