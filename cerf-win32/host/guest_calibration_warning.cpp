#include "guest_calibration_warning.h"

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/device_config.h>
#include <cerfcore/core/log.h>
#include <cerfcore/socs/guest_cpu_reset.h>
#include "guest_calibration_warning_dialog.h"
#include "host_balloon_hint.h"
#include "host_window.h"
#include <cerfcore/host/pointer_input.h>
#include <cerfcore/host/pointer_router.h>
#include <cerfcore/host/pointer_source.h>
#include <cerfcore/host/pointer_widget.h>

REGISTER_SERVICE_AS(GuestCalibrationWarning, cerfcore::CalibrationNotice);

namespace {
constexpr UINT kSwitchBackHoldMs = 5000;
}

bool GuestCalibrationWarning::ShouldRegister() {
    return emu_.Get<cerfcore::DeviceConfig>().guest_additions;
}

void GuestCalibrationWarning::OnReady() {
    emu_.Get<cerfcore::GuestCpuReset>().RegisterResetListener([this](cerfcore::ResetLineKind) {
        if (emu_.Get<cerfcore::GuestCpuReset>().DeliveredResetWasResume()) return;
        if (!present_.exchange(false)) return;
        emu_.Get<HostWindow>().RunOnUiThread([this] { EndCycle(); });
    });
}

cerfcore::PointerSource* GuestCalibrationWarning::StockSource() {
    cerfcore::PointerSource* ga = &emu_.Get<cerfcore::PointerInput>();
    for (auto* s : emu_.Get<cerfcore::PointerRouter>().Sources())
        if (s != ga) return s;
    return nullptr;
}

void GuestCalibrationWarning::OnCalibrationAppeared() {
    if (present_.exchange(true)) return;
    LOG(GuestAdditions, "[CALIB] guest calibration window appeared\n");
    emu_.Get<HostWindow>().RunOnUiThread([this] { ShowWarning(); });
}

void GuestCalibrationWarning::OnCalibrationDisappeared() {
    if (!present_.exchange(false)) return;
    LOG(GuestAdditions, "[CALIB] guest calibration window disappeared\n");
    emu_.Get<HostWindow>().RunOnUiThread([this] { EndCycle(); });
}

void GuestCalibrationWarning::ShowWarning() {
    if (emu_.Get<GuestCalibrationWarningDialog>().Show() !=
        CalibWarningChoice::SwitchToStock)
        return;

    cerfcore::PointerSource* stock = StockSource();
    if (!stock) {
        LOG(Caution, "[CALIB] no stock pointing device to switch to\n");
        return;
    }
    emu_.Get<cerfcore::PointerRouter>().SetActive(stock);
    switched_to_stock_ = true;
    LOG(GuestAdditions, "[CALIB] switched to stock input on user request\n");
}

void GuestCalibrationWarning::EndCycle() {
    emu_.Get<GuestCalibrationWarningDialog>().Dismiss();
    if (!switched_to_stock_) return;
    switched_to_stock_ = false;

    auto& router = emu_.Get<cerfcore::PointerRouter>();
    router.RearmAutoSelect();
    if (router.Active() != &emu_.Get<cerfcore::PointerInput>()) {
        LOG(GuestAdditions, "[CALIB] calibration over; the Guest Additions "
            "pointer is not ready, keeping the stock device\n");
        return;
    }
    LOG(GuestAdditions, "[CALIB] switched back to the Guest Additions pointer\n");
    emu_.Get<HostBalloonHint>().ShowUnderWidget(
        &emu_.Get<cerfcore::PointerWidget>(),
        L"Switched back to the Guest Additions mouse pointer",
        kSwitchBackHoldMs);
}
