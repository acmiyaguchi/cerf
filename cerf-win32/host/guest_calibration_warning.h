#pragma once

#include <cerfcore/host/calibration_notice.h>

#include <atomic>

namespace cerfcore { class PointerSource; }

class GuestCalibrationWarning : public cerfcore::CalibrationNotice {
public:
    using cerfcore::CalibrationNotice::CalibrationNotice;

    bool ShouldRegister() override;
    void OnReady() override;

    void OnCalibrationAppeared() override;
    void OnCalibrationDisappeared() override;

private:
    void ShowWarning();
    void EndCycle();
    cerfcore::PointerSource* StockSource();

    std::atomic<bool> present_{false};
    bool              switched_to_stock_ = false;
};
