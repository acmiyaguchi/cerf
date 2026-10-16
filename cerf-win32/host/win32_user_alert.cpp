#define NOMINMAX
#include <windows.h>

#include <cerfcore/host/user_alert.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/platform/wide_text_win32.h>
#include "host_window.h"

namespace {

class Win32UserAlert : public UserAlert {
public:
    using UserAlert::UserAlert;

    void Show(AlertLevel level, AlertPlacement placement, const std::string& title,
              const std::string& text) override {
        UINT flags = MB_OK | (level == AlertLevel::kError ? MB_ICONERROR : MB_ICONWARNING);
        HWND owner = nullptr;
        if (placement == AlertPlacement::kTopmost) flags |= MB_TASKMODAL | MB_TOPMOST;
        if (placement == AlertPlacement::kOverWindow) owner = emu_.Get<HostWindow>().Hwnd();
        MessageBoxW(owner, Utf8ToWide(text.c_str()).c_str(), Utf8ToWide(title.c_str()).c_str(),
                    flags);
    }

    void Post(AlertLevel level, const std::string& title, const std::string& text,
              std::function<void()> after) override {
        emu_.Get<HostWindow>().RunOnUiThread([this, level, title, text, after] {
            Show(level, AlertPlacement::kStandalone, title, text);
            if (after) after();
        });
    }
};

}

REGISTER_SERVICE_AS(Win32UserAlert, UserAlert);
