#pragma once

#include <cerfcore/host/keyboard_mapping_view.h>

#define NOMINMAX
#include <windows.h>

class KeyboardMappingDialog : public cerfcore::KeyboardMappingView {
public:
    using cerfcore::KeyboardMappingView::KeyboardMappingView;

    bool ShouldRegister() override;
    void OnReady() override;

    void Show() override;

private:
    static LRESULT CALLBACK WndProcStatic(HWND, UINT, WPARAM, LPARAM);
    LRESULT WndProc(HWND, UINT, WPARAM, LPARAM);

    HWND hwnd_ = nullptr;
};
