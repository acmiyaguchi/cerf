#include <cerfcore/host/configuration_problem.h>
#include <cerfcore/host/user_alert.h>

#include <cerfcore/core/cerf_emulator.h>

namespace cerffrontend {

namespace {

class LauncherConfigurationProblem : public cerfcore::ConfigurationProblem {
public:
    using cerfcore::ConfigurationProblem::ConfigurationProblem;

    void DeviceRomMissing(const std::string& device) override {
        emu_.Get<cerfcore::UserAlert>().Show(
            cerfcore::AlertLevel::kError, cerfcore::AlertPlacement::kStandalone,
            "Device not found - CE Runtime Foundation",
            "Device \"" + device +
                "\" has no ROM on disk.\n\nOpen CE Runtime Foundation Launcher to install it.");
    }

    void BoardUnsupported(const std::string& board_id) override {
        const std::string msg =
            board_id.empty()
                ? std::string(
                      "No board selected.\n\nSet \"board\": { \"id\": ... } in "
                      "cerf.json, or pass --board-id=ID. CERF will exit.")
                : ("Unrecognised board id '" + board_id +
                   "'.\n\nRun cerf.exe --help for the supported id list. "
                   "CERF will exit.");
        emu_.Get<cerfcore::UserAlert>().Show(cerfcore::AlertLevel::kError,
                                             cerfcore::AlertPlacement::kStandalone,
                                             "Unsupported board - CE Runtime Foundation", msg);
    }

    void LaunchCompactFlashMissing(const std::string& card_name) override {
        emu_.Get<cerfcore::UserAlert>().Show(
            cerfcore::AlertLevel::kWarning, cerfcore::AlertPlacement::kStandalone,
            "Missing CF Card - CE Runtime Foundation",
            "This ROM requires additional package '" + card_name +
                "' CF card to be present in the device directory. You can do "
                "that in the launcher, in the right sidepanel.\n\n"
                "The guest might not boot as intended without this card attached.");
    }
};

}

REGISTER_SERVICE_AS(LauncherConfigurationProblem, cerfcore::ConfigurationProblem);

}
