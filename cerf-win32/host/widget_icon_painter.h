#pragma once

#include <cerfcore/core/service.h>
#include <cerfcore/host/widget_icon.h>

#define NOMINMAX
#include <windows.h>

class WidgetIconPainter : public Service {
public:
    using Service::Service;

    void Paint(HDC dc, const RECT& box, const WidgetIcon& icon);

private:
    void Draw(HDC dc, const RECT& box, const ResourceIcon& icon);
    void Draw(HDC dc, const RECT& box, const ChipIcon& icon);
    void Draw(HDC dc, const RECT& box, const HardDiskIcon& icon);
    void Draw(HDC dc, const RECT& box, const LedIcon& icon);
    void Draw(HDC dc, const RECT& box, const LedGridIcon& icon);
    void Draw(HDC dc, const RECT& box, const PowerInputIcon& icon);
    void Draw(HDC dc, const RECT& box, const BatteryIcon& icon);
    void Draw(HDC dc, const RECT& box, const FolderIcon& icon);
};
