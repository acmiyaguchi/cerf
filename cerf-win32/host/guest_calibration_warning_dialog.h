#pragma once

#include "modal_dialog.h"

#include <cerffrontend/calibration_warning_prompt.h>

class GuestCalibrationWarningDialog : public ModalDialog {
public:
    using ModalDialog::ModalDialog;
    using CalibWarningChoice = cerffrontend::CalibWarningChoice;

    bool ShouldRegister() override;

    CalibWarningChoice Show();
    void               Dismiss() { PostDismiss(); }

private:
    void BuildControls(HWND hwnd) override;
    void OnPaint(HDC dc) override;
    void OnCommand(int id, int notify) override;

    int  LayoutBody(HDC dc, int width, bool draw);

    int                body_h_ = 0;
    CalibWarningChoice choice_ = CalibWarningChoice::kDismissed;
};
