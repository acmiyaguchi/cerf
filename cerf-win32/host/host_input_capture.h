#pragma once

#include <cerffrontend/input_capture.h>

#define NOMINMAX
#include <windows.h>

class HostInputCapture : public cerffrontend::InputCapture {
public:
    using cerffrontend::InputCapture::InputCapture;
    ~HostInputCapture() override;

    void AttachUiThread(HWND host_hwnd);
    void DetachUiThread();

    bool OnHookKey(WPARAM wParam, const KBDLLHOOKSTRUCT* k);

private:
    bool IsForeground() const;

    HHOOK hook_      = nullptr;
    HWND  host_hwnd_ = nullptr;
};
