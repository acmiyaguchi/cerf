#include <cerffrontend/calibration_warning_prompt.h>

#include <cerfcore/core/cerf_emulator.h>
#include "guest_calibration_warning_dialog.h"

namespace {

class Win32CalibrationWarningPrompt : public cerffrontend::CalibrationWarningPrompt {
public:
    using cerffrontend::CalibrationWarningPrompt::CalibrationWarningPrompt;

    cerffrontend::CalibWarningChoice Show() override {
        return emu_.Get<GuestCalibrationWarningDialog>().Show();
    }

    void Dismiss() override { emu_.Get<GuestCalibrationWarningDialog>().Dismiss(); }
};

}

REGISTER_SERVICE_AS(Win32CalibrationWarningPrompt, cerffrontend::CalibrationWarningPrompt);
