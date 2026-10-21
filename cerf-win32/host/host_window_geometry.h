#pragma once

#include <cerfcore/core/service.h>

#define NOMINMAX
#include <windows.h>

#include <cstdint>

class HostWindowGeometry : public cerfcore::Service {
public:
    using cerfcore::Service::Service;

    void ChromeExtent(HWND hwnd, UINT dpi, int& extra_w, int& extra_h) const;

    bool AdoptSurfaceForMonitor(HWND hwnd, uint32_t& w, uint32_t& h);

    void FitToSurface(HWND hwnd, uint32_t sw, uint32_t sh) const;
};
