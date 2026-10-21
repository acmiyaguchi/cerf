#define NOMINMAX
#include <cerffrontend/surface_factory.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/fatal.h>
#include "win32_painter.h"

#include <windows.h>

#include <algorithm>
#include <memory>

namespace {

class Win32OffscreenSurface final : public cerffrontend::OffscreenSurface {
public:
    Win32OffscreenSurface(cerfcore::CerfEmulator& emu, int width, int height)
        : w_(std::max(width, 1)), h_(std::max(height, 1)) {
        BITMAPINFO bmi              = {};
        bmi.bmiHeader.biSize        = sizeof(bmi.bmiHeader);
        bmi.bmiHeader.biWidth       = w_;
        bmi.bmiHeader.biHeight      = -h_;
        bmi.bmiHeader.biPlanes      = 1;
        bmi.bmiHeader.biBitCount    = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        void* bits = nullptr;
        dib_ = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
        dc_  = CreateCompatibleDC(nullptr);
        if (!dib_ || !bits || !dc_)
            emu.Get<cerfcore::Fatal>().Die(
                "Win32OffscreenSurface: %dx%d DIB section failed (gle=%lu)", w_, h_,
                GetLastError());
        old_ = SelectObject(dc_, dib_);
        bits_    = static_cast<uint32_t*>(bits);
        painter_ = std::make_unique<Win32Painter>(emu, dc_);
    }

    ~Win32OffscreenSurface() override {
        SelectObject(dc_, old_);
        DeleteDC(dc_);
        DeleteObject(dib_);
    }

    Win32OffscreenSurface(const Win32OffscreenSurface&)            = delete;
    Win32OffscreenSurface& operator=(const Win32OffscreenSurface&) = delete;

    uint32_t*               Bits() override { return bits_; }
    int                     Width() const override { return w_; }
    int                     Height() const override { return h_; }
    cerffrontend::Painter&  Paint() override { return *painter_; }
    void                    Flush() override { GdiFlush(); }

private:
    int                           w_;
    int                           h_;
    HBITMAP                       dib_  = nullptr;
    HDC                           dc_   = nullptr;
    HGDIOBJ                       old_  = nullptr;
    uint32_t*                     bits_ = nullptr;
    std::unique_ptr<Win32Painter> painter_;
};

class Win32SurfaceFactory : public cerffrontend::SurfaceFactory {
public:
    using cerffrontend::SurfaceFactory::SurfaceFactory;

    std::unique_ptr<cerffrontend::OffscreenSurface> Create(int width, int height) override {
        return std::make_unique<Win32OffscreenSurface>(emu_, width, height);
    }
};

}

REGISTER_SERVICE_AS(Win32SurfaceFactory, cerffrontend::SurfaceFactory);
