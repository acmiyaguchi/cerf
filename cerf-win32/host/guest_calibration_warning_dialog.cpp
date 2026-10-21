#define NOMINMAX
#include "guest_calibration_warning_dialog.h"

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/device_config.h>
#include <cerfcore/platform/wide_text_win32.h>
#include <cerffrontend/calibration_warning_body.h>
#include <cerffrontend/launcher_customizations_editor.h>
#include "host_dark_mode.h"
#include "host_window.h"
#include "win32_painter.h"

REGISTER_SERVICE(GuestCalibrationWarningDialog);

namespace {

using Body = cerffrontend::CalibrationWarningBody;

constexpr wchar_t kClass[] = L"CerfGuestCalibrationWarningDlg";

enum : int {
    IDC_CHANGE_RES = 4102,
};

}

bool GuestCalibrationWarningDialog::ShouldRegister() {
    return emu_.Get<cerfcore::DeviceConfig>().guest_additions;
}

int GuestCalibrationWarningDialog::LayoutBody(HDC dc, int width, bool draw) {
    auto&          dark_mode = emu_.Get<HostDarkMode>();
    const bool     dark      = dark_mode.IsDark();
    const COLORREF text      = dark ? dark_mode.TextColor() : GetSysColor(COLOR_BTNTEXT);
    Win32Painter   painter(emu_, dc);
    return Body::Layout(painter, width, draw, dark,
                        cerffrontend::Rgb{GetRValue(text), GetGValue(text), GetBValue(text)});
}

void GuestCalibrationWarningDialog::OnPaint(HDC dc) {
    LayoutBody(dc, Body::kClientW, true);
}

void GuestCalibrationWarningDialog::BuildControls(HWND hwnd) {
    HINSTANCE inst = GetModuleHandleW(nullptr);
    const int by = body_h_ + Body::kButtonTop;
    auto mk = [&](const char* text, DWORD style, int x, int w, int id) {
        return CreateWindowExW(0, L"BUTTON", cerfcore::Utf8ToWide(text).c_str(),
                               WS_CHILD | WS_VISIBLE | style, x, by, w, Body::kButtonH, hwnd,
                               (HMENU)(INT_PTR)id, inst, nullptr);
    };

    const int close_x  = Body::kClientW - Body::kMargin - Body::kCloseW;
    const int chres_x  = close_x - Body::kButtonGap - Body::kChangeResW;
    const int switch_x = chres_x - Body::kButtonGap - Body::kSwitchW;

    mk(Body::kSwitchLabel, BS_DEFPUSHBUTTON | WS_TABSTOP | WS_GROUP, switch_x, Body::kSwitchW,
       IDOK);
    mk(Body::kChangeLabel, BS_PUSHBUTTON | WS_TABSTOP, chres_x, Body::kChangeResW, IDC_CHANGE_RES);
    mk(Body::kCloseLabel, BS_PUSHBUTTON | WS_TABSTOP, close_x, Body::kCloseW, IDCANCEL);

    HFONT gui = emu_.Get<HostDarkMode>().UiFont();
    if (!gui) gui = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    for (HWND c = GetWindow(hwnd, GW_CHILD); c; c = GetWindow(c, GW_HWNDNEXT))
        SendMessageW(c, WM_SETFONT, (WPARAM)gui, TRUE);
}

void GuestCalibrationWarningDialog::OnCommand(int id, int) {
    if (id == IDOK) {
        choice_ = CalibWarningChoice::kSwitchToStock;
        Finish();
    } else if (id == IDC_CHANGE_RES) {
        if (emu_.Get<cerffrontend::LauncherCustomizationsEditor>().Open(
                reinterpret_cast<uintptr_t>(Hwnd()), true))
            Finish();
    } else if (id == IDCANCEL) {
        Finish();
    }
}

GuestCalibrationWarningDialog::CalibWarningChoice GuestCalibrationWarningDialog::Show() {
    if (IsOpen()) return CalibWarningChoice::kDismissed;

    choice_ = CalibWarningChoice::kDismissed;

    HDC screen = GetDC(nullptr);
    body_h_ = LayoutBody(screen, Body::kClientW, false);
    ReleaseDC(nullptr, screen);

    RunModal(emu_.Get<HostWindow>().Hwnd(), kClass,
             cerfcore::Utf8ToWide(Body::kTitle).c_str(), Body::kClientW,
             body_h_ + Body::kButtonRow);
    return choice_;
}
