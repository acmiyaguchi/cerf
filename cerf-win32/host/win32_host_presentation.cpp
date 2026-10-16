#include <cerfcore/host/host_presentation.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/platform/wide_text_win32.h>
#include "boot_screen.h"
#include "host_canvas.h"
#include "host_screenshot.h"
#include "host_window.h"

namespace {

class Win32HostPresentation : public cerfcore::HostPresentation {
public:
    using cerfcore::HostPresentation::HostPresentation;

    void PanelEnabled() override { emu_.Get<HostWindow>().OnLcdEnabled(); }

    void GuestRemoded(uint32_t guest_w, uint32_t guest_h) override {
        emu_.Get<HostWindow>().NotifyGuestRemoded(guest_w, guest_h);
    }

    uint32_t SurfaceWidth() const override { return emu_.Get<HostCanvas>().GuestSurfaceWidth(); }
    uint32_t SurfaceHeight() const override { return emu_.Get<HostCanvas>().GuestSurfaceHeight(); }

    void RestoreSurfaceSize(uint32_t w, uint32_t h) override {
        emu_.Get<HostWindow>().RunOnUiThread([this, w, h] {
            emu_.Get<HostCanvas>().SetGuestSurfaceSize(w, h);
            emu_.Get<HostWindow>().MatchGuestSize();
        });
    }

    void SaveScreenshot(const std::string& path) override {
        const std::wstring wide = cerfcore::Utf8ToWide(path.c_str());
        emu_.Get<HostWindow>().RunOnUiThread([this, wide] {
            emu_.Get<HostScreenshot>().SaveGuestSurfaceTo(wide);
        });
    }

    void ShowCustomizationsRestart() override {
        emu_.Get<BootScreen>().SetRestartLabel(L"Applying customizations...");
    }
};

}

REGISTER_SERVICE_AS(Win32HostPresentation, cerfcore::HostPresentation);
