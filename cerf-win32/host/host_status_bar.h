#pragma once

#include <cerfcore/core/service.h>
#include <cerffrontend/status_bar.h>
#include "win32_menu_binder.h"

#define NOMINMAX
#include <windows.h>
#include <commctrl.h>

#include <string>
#include <vector>

namespace cerfcore { class HostWidget; }

class HostStatusBar : public cerfcore::Service {
public:
    using cerfcore::Service::Service;

    void OnReady() override;

    int Height() const { return cerffrontend::StatusBar::kHeight; }

    void CreateOn(HWND parent, const RECT& rect);
    void Reposition(const RECT& rect);

    HWND Hwnd() const { return hwnd_; }

    bool WidgetScreenRect(const cerfcore::HostWidget* w, RECT& out) const;
    bool CaptureWidgetScreenRect(RECT& out) const;

private:
    static LRESULT CALLBACK WndProcStatic(HWND, UINT, WPARAM, LPARAM);
    LRESULT WndProc(HWND, UINT, WPARAM, LPARAM);

    void RebuildTooltips();
    void UpdateTipText(size_t idx, std::wstring text);
    void SecondaryAction(cerfcore::HostWidget* w, int x, int y, UINT button_flag);
    bool ToScreen(const cerffrontend::Rect& box, RECT& out) const;

    HWND                      hwnd_      = nullptr;
    HWND                      tip_hwnd_  = nullptr;
    std::vector<std::wstring> tip_texts_;
    size_t                    tip_count_ = 0;
    Win32MenuBinder           binder_;
};
