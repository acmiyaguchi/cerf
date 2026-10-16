#pragma once

#include <cerfcore/host/calibration_notice.h>

#include <atomic>

class PointerSource;

class GuestCalibrationWarning : public CalibrationNotice {
public:
    using CalibrationNotice::CalibrationNotice;

    bool ShouldRegister() override;
    void OnReady() override;

    void OnCalibrationAppeared() override;
    void OnCalibrationDisappeared() override;

private:
    void ShowWarning();
    void EndCycle();
    PointerSource* StockSource();

    std::atomic<bool> present_{false};
    bool              switched_to_stock_ = false;
};
