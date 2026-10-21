#pragma once

#include <cerffrontend/painter.h>

#include <cerfcore/core/service.h>

#define NOMINMAX
#include <windows.h>

#include <map>
#include <mutex>
#include <tuple>

namespace Gdiplus { class Bitmap; }

class Win32PaintResources : public cerfcore::Service {
public:
    using cerfcore::Service::Service;
    ~Win32PaintResources() override;

    void OnShutdown() override;

    HFONT            Font(const cerffrontend::Font& font);
    Gdiplus::Bitmap* Image(cerffrontend::PainterImage image);

private:
    std::mutex                                         mutex_;
    std::map<std::tuple<uint8_t, int, bool>, HFONT>    fonts_;
    Gdiplus::Bitmap*                                   logo_        = nullptr;
    bool                                               logo_loaded_ = false;
};
