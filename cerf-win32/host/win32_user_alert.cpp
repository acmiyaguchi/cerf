#define NOMINMAX
#include <windows.h>

#include <cerfcore/host/user_alert.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/platform/wide_text_win32.h>
#include "host_window.h"

namespace {

class Win32UserAlert : public cerfcore::UserAlert {
public:
    using cerfcore::UserAlert::UserAlert;

    void Show(cerfcore::AlertLevel level, cerfcore::AlertPlacement placement,
              const std::string& title, const std::string& text) override {
        UINT flags =
            MB_OK | (level == cerfcore::AlertLevel::kError ? MB_ICONERROR : MB_ICONWARNING);
        HWND owner = nullptr;
        if (placement == cerfcore::AlertPlacement::kTopmost) flags |= MB_TASKMODAL | MB_TOPMOST;
        if (placement == cerfcore::AlertPlacement::kOverWindow)
            owner = emu_.Get<HostWindow>().Hwnd();
        MessageBoxW(owner, cerfcore::Utf8ToWide(text.c_str()).c_str(),
                    cerfcore::Utf8ToWide(title.c_str()).c_str(), flags);
    }

    void Post(cerfcore::AlertLevel level, const std::string& title, const std::string& text,
              std::function<void()> after) override {
        emu_.Get<HostWindow>().RunOnUiThread([this, level, title, text, after] {
            Show(level, cerfcore::AlertPlacement::kStandalone, title, text);
            if (after) after();
        });
    }
};

}

REGISTER_SERVICE_AS(Win32UserAlert, cerfcore::UserAlert);
