#include <cerffrontend/launcher_customizations_editor.h>

#include <cerffrontend/frontend_window.h>
#include <cerffrontend/guest_additions_ui_policy.h>
#include <cerffrontend/launcher_transaction.h>

#include <cerfcore/boot/guest_cold_boot.h>
#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/device_config.h>
#include <cerfcore/core/device_config_refresh.h>
#include <cerfcore/peripherals/cerf_virt/cerf_virt_customizations_reset.h>
#include <cerfcore/peripherals/cerf_virt/cerf_virt_resize.h>
#include <cerfcore/socs/guest_cpu_reset.h>

namespace cerffrontend {

REGISTER_SERVICE_AS(LauncherCustomizationsEditor, cerfcore::CustomizationsEditor);

bool LauncherCustomizationsEditor::ShouldRegister() {
    return emu_.Get<cerfcore::DeviceConfig>().guest_additions;
}

void LauncherCustomizationsEditor::Open() {
    Open(emu_.Get<FrontendWindow>().NativeHandle(), false);
}

bool LauncherCustomizationsEditor::Open(uint64_t owner, bool force_reboot) {
    nlohmann::json query;
    query["force_reboot"] = force_reboot;
    uint16_t major = 0, minor = 0;
    if (emu_.Get<GuestAdditionsUiPolicy>().CeVersion(major, minor))
        query["ce_version"] = {{"major", major}, {"minor", minor}};

    auto&          cfg    = emu_.Get<cerfcore::DeviceConfig>();
    const uint32_t prev_w = cfg.board_configurable_screen_width;
    const uint32_t prev_h = cfg.board_configurable_screen_height;

    nlohmann::json response;
    if (!emu_.Get<LauncherTransaction>().Run(owner, "live_customizations", query, response))
        return false;

    emu_.Get<cerfcore::DeviceConfigRefresh>().Refresh();

    if (!response.is_object() || !response.contains("reboot")) return false;

    emu_.Get<cerfcore::CerfVirtCustomizationsReset>().Invalidate();

    std::string reboot;
    if (response["reboot"].is_string()) reboot = response["reboot"].get<std::string>();
    Apply(reboot, prev_w, prev_h);
    return true;
}

void LauncherCustomizationsEditor::Apply(const std::string& reboot, uint32_t prev_w,
                                         uint32_t prev_h) {
    auto&          cfg = emu_.Get<cerfcore::DeviceConfig>();
    const uint32_t w   = cfg.board_configurable_screen_width;
    const uint32_t h   = cfg.board_configurable_screen_height;

    auto& win = emu_.Get<FrontendWindow>();
    if (reboot == "soft") {
        win.SetGuestResolution(w, h);
        win.FitToResolution(w, h);
        emu_.Get<cerfcore::GuestCpuReset>().WarmReset();
    } else if (reboot == "hard") {
        win.SetGuestResolution(w, h);
        win.FitToResolution(w, h);
        emu_.Get<cerfcore::GuestColdBoot>().RequestHardReset();
    } else if (w != prev_w || h != prev_h) {
        win.FitToResolution(w, h);
        emu_.Get<cerfcore::CerfVirtResize>().RequestResize(w, h);
    }
}

}
