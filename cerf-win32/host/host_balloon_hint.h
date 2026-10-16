#pragma once

#include <cerfcore/core/service.h>

#define NOMINMAX
#include <windows.h>

#include <string>

namespace cerfcore { class HostWidget; }

class HostBalloonHint : public cerfcore::Service {
public:
    using cerfcore::Service::Service;

    void OnReady() override;
    void OnShutdown() override;

    void ShowUnderWidget(const cerfcore::HostWidget* anchor, const wchar_t* text,
                         UINT hold_ms);
    void ShowUnderCaptureWidget(const wchar_t* text, UINT hold_ms);

private:
    static LRESULT CALLBACK WndProcStatic(HWND, UINT, WPARAM, LPARAM);

    void Show(const RECT* anchor, const wchar_t* text, UINT hold_ms);
    void Dismiss();

    HWND host_ = nullptr;
    HWND tip_  = nullptr;
    std::wstring shown_text_;
};
