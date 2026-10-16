#include <cerfcore/host/device_selection.h>
#include <cerffrontend/frontend_config.h>
#include <cerfcore/host/user_alert.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/log.h>

namespace cerffrontend {

namespace {

class LauncherDeviceSelection : public cerfcore::DeviceSelection {
public:
    using cerfcore::DeviceSelection::DeviceSelection;

    std::string SelectedDevice() override {
        const char* cli = emu_.Config().device_override;
        if (cli && cli[0]) return cli;

        auto& config = emu_.Get<FrontendConfig>();
        if (!config.Device().empty()) return config.Device();

        LOG(Caution, "device is blank: set \"device\" in %s or pass --device=NAME\n",
            config.Path().c_str());
#if !CERF_DEV_MODE
        emu_.Get<cerfcore::UserAlert>().Show(
            cerfcore::AlertLevel::kError, cerfcore::AlertPlacement::kStandalone,
            "No device - CE Runtime Foundation",
            "No device is selected. CERF will exit.\n\nStart a device from CE Runtime "
            "Foundation Launcher, or start cerf.exe with --device=NAME.");
#endif
        cerfcore::CerfFatalExit(cerfcore::CERF_FATAL_USER_ERROR);
    }
};

}

REGISTER_SERVICE_AS(LauncherDeviceSelection, cerfcore::DeviceSelection);

}
