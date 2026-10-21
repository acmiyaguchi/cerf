#pragma once

#include <cerfcore/core/service.h>
#include <cerffrontend/canvas_input.h>

#define NOMINMAX
#include <windows.h>

class HostCanvasInput : public cerfcore::Service, public cerffrontend::CanvasInputHost {
public:
    using cerfcore::Service::Service;

    void OnReady() override;

    bool Handle(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, LRESULT& out);

    void SetMouseCapture(bool on) override;
    bool HasMouseCapture() const override;
    void FocusCanvas() override;
    void SetPointerHidden(bool hidden) override;
    void WarpPointerTo(int x, int y) override;
    void ClientSize(int& width, int& height) const override;
    bool HostToGuest(int x, int y, int& sx, int& sy) const override;
    void ClampGuest(int& sx, int& sy) const override;

private:
    bool SetCursorFor(HWND hwnd, LRESULT& out);
};
