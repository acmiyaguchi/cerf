#include "host_window_geometry.h"

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/device_config.h>
#include <cerfcore/core/log.h>
#include <cerfcore/peripherals/cerf_virt/cerf_virt_framebuffer.h>
#include "host_dpi.h"
#include "host_status_bar.h"

REGISTER_SERVICE(HostWindowGeometry);

void HostWindowGeometry::ChromeExtent(HWND hwnd, UINT dpi, int& extra_w, int& extra_h) const {
    const DWORD style = (DWORD)GetWindowLongW(hwnd, GWL_STYLE);
    const DWORD ex    = (DWORD)GetWindowLongW(hwnd, GWL_EXSTYLE);
    RECT r = { 0, 0, 0, 0 };
    emu_.Get<HostDpi>().AdjustForDpi(r, style, TRUE, ex, dpi);
    extra_w = (int)(r.right - r.left);
    extra_h = (int)(r.bottom - r.top)
            + (int)emu_.Get<HostStatusBar>().Height();
}

bool HostWindowGeometry::AdoptSurfaceForMonitor(HWND hwnd, uint32_t& w, uint32_t& h) {
    auto& dc = emu_.Get<cerfcore::DeviceConfig>();
    if (!dc.guest_additions ||
        !dc.adopt_guest_additions_resolution_for_host_screen)
        return false;

    MONITORINFO mi = { sizeof(mi) };
    if (!GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi))
        return false;
    const int work_w = (int)(mi.rcWork.right  - mi.rcWork.left);
    const int work_h = (int)(mi.rcWork.bottom - mi.rcWork.top);

    int ex_w = 0, ex_h = 0;
    ChromeExtent(hwnd, emu_.Get<HostDpi>().ForWindow(hwnd), ex_w, ex_h);
    const int surf_w = work_w - ex_w;
    const int surf_h = work_h - ex_h;
    if (surf_w < 1 || surf_h < 1) return false;

    emu_.Get<cerfcore::CerfVirtFramebuffer>().ApplyGuestMode((uint32_t)surf_w, (uint32_t)surf_h);
    w = (uint32_t)surf_w;
    h = (uint32_t)surf_h;
    LOG(Lcd, "HostWindow: adopt fit monitor work=%dx%d -> guest surface %dx%d\n",
        work_w, work_h, surf_w, surf_h);
    return true;
}

void HostWindowGeometry::FitToSurface(HWND hwnd, uint32_t sw, uint32_t sh) const {
    int ex_w = 0, ex_h = 0;
    ChromeExtent(hwnd, emu_.Get<HostDpi>().ForWindow(hwnd), ex_w, ex_h);
    int outer_w = (int)sw + ex_w;
    int outer_h = (int)sh + ex_h;

    int x = 0, y = 0;
    bool move = false;
    MONITORINFO mi = { sizeof(mi) };
    if (GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi)) {
        const RECT wa = mi.rcWork;
        const int wa_w = (int)(wa.right - wa.left);
        const int wa_h = (int)(wa.bottom - wa.top);
        if (outer_w > wa_w) outer_w = wa_w;
        if (outer_h > wa_h) outer_h = wa_h;

        x = (int)wa.left + (wa_w - outer_w) / 2;
        y = (int)wa.top  + (wa_h - outer_h) / 2;
        move = true;
    }

    if (IsZoomed(hwnd)) ShowWindow(hwnd, SW_SHOWNOACTIVATE);
    SetWindowPos(hwnd, nullptr, x, y, outer_w, outer_h,
                 SWP_NOZORDER | SWP_NOACTIVATE | (move ? 0u : SWP_NOMOVE));
}
