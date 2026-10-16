#pragma once

#include <cerfcore/core/service.h>
#include <cerfcore/host/widget_icon.h>

#define NOMINMAX
#include <windows.h>

class WidgetIconPainter : public cerfcore::Service {
public:
    using cerfcore::Service::Service;

    void Paint(HDC dc, const RECT& box, const cerfcore::WidgetIcon& icon);

private:
    void Draw(HDC dc, const RECT& box, const cerfcore::ResourceIcon& icon);
    void Draw(HDC dc, const RECT& box, const cerfcore::ChipIcon& icon);
    void Draw(HDC dc, const RECT& box, const cerfcore::HardDiskIcon& icon);
    void Draw(HDC dc, const RECT& box, const cerfcore::LedIcon& icon);
    void Draw(HDC dc, const RECT& box, const cerfcore::LedGridIcon& icon);
    void Draw(HDC dc, const RECT& box, const cerfcore::PowerInputIcon& icon);
    void Draw(HDC dc, const RECT& box, const cerfcore::BatteryIcon& icon);
    void Draw(HDC dc, const RECT& box, const cerfcore::FolderIcon& icon);
};
