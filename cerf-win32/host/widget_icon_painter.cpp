#include "widget_icon_painter.h"

#include <cerfcore/core/cerf_emulator.h>
#include "host_gdiplus.h"
#include "host_icon_cache.h"

REGISTER_SERVICE(WidgetIconPainter);

namespace {

COLORREF Rgb(const cerfcore::WidgetRgb& c) { return RGB(c.r, c.g, c.b); }

}

void WidgetIconPainter::Paint(HDC dc, const RECT& box, const cerfcore::WidgetIcon& icon) {
    std::visit([&](const auto& i) { Draw(dc, box, i); }, icon);
}

void WidgetIconPainter::Draw(HDC dc, const RECT& box, const cerfcore::ResourceIcon& icon) {
    emu_.Get<HostIconCache>().DrawCentered(dc, box, icon.name);
}

void WidgetIconPainter::Draw(HDC dc, const RECT& box, const cerfcore::ChipIcon&) {
    const int cx = (box.left + box.right) / 2;
    const int cy = (box.top + box.bottom) / 2;
    const RECT body = { cx - 8, cy - 6, cx + 8, cy + 6 };

    HBRUSH  fill = CreateSolidBrush(RGB(40, 44, 52));
    HPEN    pen  = CreatePen(PS_SOLID, 1, RGB(150, 150, 160));
    HGDIOBJ ob   = SelectObject(dc, fill);
    HGDIOBJ op   = SelectObject(dc, pen);
    Rectangle(dc, body.left, body.top, body.right, body.bottom);
    for (int i = -1; i <= 1; ++i) {
        const int px = cx + i * 5;
        MoveToEx(dc, px, body.top - 2, nullptr);    LineTo(dc, px, body.top);
        MoveToEx(dc, px, body.bottom - 1, nullptr); LineTo(dc, px, body.bottom + 1);
    }
    SelectObject(dc, ob);
    SelectObject(dc, op);
    DeleteObject(fill);
    DeleteObject(pen);
}

void WidgetIconPainter::Draw(HDC dc, const RECT& box, const cerfcore::HardDiskIcon&) {
    const int cx = (box.left + box.right) / 2;
    const int cy = (box.top + box.bottom) / 2;
    RECT body = { cx - 8, cy - 6, cx + 8, cy + 6 };

    HBRUSH  fill = CreateSolidBrush(RGB(48, 52, 60));
    HPEN    pen  = CreatePen(PS_SOLID, 1, RGB(150, 150, 160));
    HGDIOBJ ob   = SelectObject(dc, fill);
    HGDIOBJ op   = SelectObject(dc, pen);
    RoundRect(dc, body.left, body.top, body.right, body.bottom, 3, 3);
    HBRUSH spindle = CreateSolidBrush(RGB(170, 175, 185));
    HGDIOBJ os = SelectObject(dc, spindle);
    Ellipse(dc, cx - 2, cy - 2, cx + 3, cy + 3);
    SelectObject(dc, os);
    DeleteObject(spindle);
    SelectObject(dc, ob);
    SelectObject(dc, op);
    DeleteObject(fill);
    DeleteObject(pen);
}

void WidgetIconPainter::Draw(HDC dc, const RECT& box, const cerfcore::LedIcon& icon) {
    const int cx = (box.left + box.right) / 2;
    const int cy = (box.top + box.bottom) / 2;
    const int r  = icon.radius;

    if (icon.vibrate) {
        HPEN vp = CreatePen(PS_SOLID, 1, RGB(200, 160, 80));
        HGDIOBJ ov = SelectObject(dc, vp);
        for (int s = 1; s <= 2; ++s) {
            const int xr = cx + r + 1 + s * 2;
            MoveToEx(dc, xr, cy - 3, nullptr); LineTo(dc, xr + 2, cy);
            LineTo(dc, xr, cy + 3);
            const int xl = cx - r - 1 - s * 2;
            MoveToEx(dc, xl, cy - 3, nullptr); LineTo(dc, xl - 2, cy);
            LineTo(dc, xl, cy + 3);
        }
        SelectObject(dc, ov);
        DeleteObject(vp);
    }

    emu_.Get<HostGdiPlus>().FillCircleAA(dc, cx, cy, r, Rgb(icon.fill), Rgb(icon.rim));
}

void WidgetIconPainter::Draw(HDC dc, const RECT& box, const cerfcore::LedGridIcon& icon) {
    constexpr int kCell = 4, kGap = 1;
    const int grid_w = 4 * kCell + 3 * kGap;
    const int grid_h = 2 * kCell + 1 * kGap;
    const int x0 = (box.left + box.right) / 2 - grid_w / 2;
    const int y0 = (box.top + box.bottom) / 2 - grid_h / 2;

    HPEN    pen = CreatePen(PS_SOLID, 1, Rgb(icon.outline));
    HGDIOBJ op  = SelectObject(dc, pen);
    for (int i = 0; i < 8; ++i) {
        const bool on  = ((icon.bits >> (7 - i)) & 1u) != 0u;
        const int  col = i % 4;
        const int  row = i / 4;
        const int  x   = x0 + col * (kCell + kGap);
        const int  y   = y0 + row * (kCell + kGap);
        HBRUSH  fill = CreateSolidBrush(Rgb(on ? icon.lit : icon.dark));
        HGDIOBJ ob   = SelectObject(dc, fill);
        Rectangle(dc, x, y, x + kCell, y + kCell);
        SelectObject(dc, ob);
        DeleteObject(fill);
    }
    SelectObject(dc, op);
    DeleteObject(pen);
}

void WidgetIconPainter::Draw(HDC dc, const RECT& box, const cerfcore::PowerInputIcon& icon) {
    const COLORREF outline = icon.failed ? RGB(245, 105, 105) : RGB(155, 165, 175);
    const COLORREF fill = icon.failed ? RGB(105, 28, 28) : RGB(38, 48, 58);
    const int cx = (box.left + box.right) / 2;
    const int cy = (box.top + box.bottom) / 2;

    HPEN pen = CreatePen(PS_SOLID, 2, outline);
    HBRUSH brush = CreateSolidBrush(fill);
    HGDIOBJ old_pen = SelectObject(dc, pen);
    HGDIOBJ old_brush = SelectObject(dc, brush);
    Rectangle(dc, cx - 5, cy - 5, cx + 5, cy + 5);
    MoveToEx(dc, cx - 2, cy - 9, nullptr);
    LineTo(dc, cx - 2, cy - 5);
    MoveToEx(dc, cx + 2, cy - 9, nullptr);
    LineTo(dc, cx + 2, cy - 5);
    MoveToEx(dc, cx, cy + 5, nullptr);
    LineTo(dc, cx, cy + 9);
    SelectObject(dc, old_brush);
    SelectObject(dc, old_pen);
    DeleteObject(brush);
    DeleteObject(pen);
}

void WidgetIconPainter::Draw(HDC dc, const RECT& box, const cerfcore::BatteryIcon& icon) {
    const COLORREF lvl = icon.level == cerfcore::BatteryLevel::kHigh ? RGB(78, 201, 90)
                       : icon.level == cerfcore::BatteryLevel::kLow  ? RGB(220, 200, 60)
                                                           : RGB(229, 80, 80);

    const int cx = (box.left + box.right) / 2;
    const int cy = (box.top + box.bottom) / 2;
    constexpr int kBodyW = 18, kBodyH = 11;
    RECT body = { cx - kBodyW / 2, cy - kBodyH / 2,
                  cx + kBodyW / 2, cy + kBodyH / 2 };

    HPEN    pen   = CreatePen(PS_SOLID, 1, RGB(170, 170, 180));
    HBRUSH  dark  = CreateSolidBrush(RGB(30, 32, 38));
    HGDIOBJ op    = SelectObject(dc, pen);
    HGDIOBJ ob    = SelectObject(dc, dark);
    Rectangle(dc, body.left, body.top, body.right, body.bottom);

    RECT nub = { body.right, cy - 2, body.right + 2, cy + 2 };
    HBRUSH nubfill = CreateSolidBrush(RGB(170, 170, 180));
    FillRect(dc, &nub, nubfill);
    DeleteObject(nubfill);

    const int inner_w = kBodyW - 2;
    const int fw      = inner_w * icon.fill_percent / 100;
    if (fw > 0) {
        RECT fr = { body.left + 1, body.top + 1, body.left + 1 + fw, body.bottom - 1 };
        HBRUSH fb = CreateSolidBrush(lvl);
        FillRect(dc, &fr, fb);
        DeleteObject(fb);
    }

    if (!icon.on_battery) {
        const POINT bolt[] = {
            { cx,     cy - 5 }, { cx - 3, cy + 1 }, { cx,     cy + 1 },
            { cx - 1, cy + 5 }, { cx + 3, cy - 1 }, { cx,     cy - 1 },
        };
        emu_.Get<HostGdiPlus>().FillPolygonAA(
            dc, bolt, (int)(sizeof(bolt) / sizeof(bolt[0])),
            RGB(120, 220, 255), RGB(40, 60, 80));
    }

    SelectObject(dc, ob);
    SelectObject(dc, op);
    DeleteObject(dark);
    DeleteObject(pen);
}

void WidgetIconPainter::Draw(HDC dc, const RECT& box, const cerfcore::FolderIcon& icon) {
    const COLORREF clr = icon.enabled ? RGB(232, 196, 92) : RGB(224, 224, 224);
    const int cx = (box.left + box.right) / 2;
    const int cy = (box.top + box.bottom) / 2;
    const POINT folder[6] = {
        { cx - 8, cy + 6 }, { cx - 8, cy - 6 }, { cx - 2, cy - 6 },
        { cx,     cy - 3 }, { cx + 8, cy - 3 }, { cx + 8, cy + 6 },
    };
    emu_.Get<HostGdiPlus>().FillPolygonAA(dc, folder, 6, clr, clr);
}
