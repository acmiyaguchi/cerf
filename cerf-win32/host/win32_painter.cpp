#define NOMINMAX
#include "win32_painter.h"

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/platform/wide_text_win32.h>
#include "host_gdiplus.h"
#include "host_icon_cache.h"
#include "win32_paint_resources.h"

#include <objbase.h>
#include <gdiplus.h>

#include <string>
#include <vector>

namespace {

COLORREF Colorref(cerffrontend::Rgb c) { return RGB(c.r, c.g, c.b); }

RECT Win32Rect(const cerffrontend::Rect& r) { return RECT{r.left, r.top, r.right, r.bottom}; }

std::wstring Wide(std::string_view utf8) { return cerfcore::Utf8ToWide(std::string(utf8).c_str()); }

UINT DrawTextFlags(uint32_t flags) {
    UINT out = DT_NOPREFIX;
    if (flags & cerffrontend::text::kCenter)      out |= DT_CENTER;
    if (flags & cerffrontend::text::kVCenter)     out |= DT_VCENTER;
    if (flags & cerffrontend::text::kBottom)      out |= DT_BOTTOM;
    if (flags & cerffrontend::text::kWordBreak)   out |= DT_WORDBREAK;
    if (flags & cerffrontend::text::kSingleLine)  out |= DT_SINGLELINE;
    if (flags & cerffrontend::text::kEndEllipsis) out |= DT_END_ELLIPSIS;
    if (flags & cerffrontend::text::kEditControl) out |= DT_EDITCONTROL;
    return out;
}

std::vector<POINT> Win32Points(const cerffrontend::Point* points, int count) {
    std::vector<POINT> out(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) out[static_cast<size_t>(i)] = POINT{points[i].x, points[i].y};
    return out;
}

}

void Win32Painter::FillRect(const cerffrontend::Rect& r, cerffrontend::Rgb color) {
    const RECT   rc    = Win32Rect(r);
    const HBRUSH brush = CreateSolidBrush(Colorref(color));
    ::FillRect(dc_, &rc, brush);
    DeleteObject(brush);
}

void Win32Painter::BlendRect(const cerffrontend::Rect& r, cerffrontend::Rgb color, uint8_t alpha) {
    HDC mdc = CreateCompatibleDC(dc_);
    if (!mdc) return;
    BITMAPINFO bi              = {};
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = 1;
    bi.bmiHeader.biHeight      = 1;
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    void*   bits = nullptr;
    HBITMAP dib  = CreateDIBSection(mdc, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (dib && bits) {
        *static_cast<uint32_t*>(bits) =
            (static_cast<uint32_t>(color.r) << 16) | (static_cast<uint32_t>(color.g) << 8) | color.b;
        HGDIOBJ       ob = SelectObject(mdc, dib);
        BLENDFUNCTION bf = {AC_SRC_OVER, 0, alpha, 0};
        AlphaBlend(dc_, r.left, r.top, r.Width(), r.Height(), mdc, 0, 0, 1, 1, bf);
        SelectObject(mdc, ob);
        DeleteObject(dib);
    }
    DeleteDC(mdc);
}

void Win32Painter::Rectangle(const cerffrontend::Rect& r, cerffrontend::Rgb pen, int pen_width,
                             const cerffrontend::Rgb* fill) {
    HPEN    p     = CreatePen(PS_SOLID, pen_width, Colorref(pen));
    HBRUSH  brush = fill ? CreateSolidBrush(Colorref(*fill)) : nullptr;
    HGDIOBJ op    = SelectObject(dc_, p);
    HGDIOBJ ob    = SelectObject(dc_, brush ? brush : GetStockObject(NULL_BRUSH));
    ::Rectangle(dc_, r.left, r.top, r.right, r.bottom);
    SelectObject(dc_, ob);
    SelectObject(dc_, op);
    if (brush) DeleteObject(brush);
    DeleteObject(p);
}

void Win32Painter::RoundRect(const cerffrontend::Rect& r, int corner_w, int corner_h,
                             cerffrontend::Rgb pen, cerffrontend::Rgb fill) {
    HPEN    p     = CreatePen(PS_SOLID, 1, Colorref(pen));
    HBRUSH  brush = CreateSolidBrush(Colorref(fill));
    HGDIOBJ op    = SelectObject(dc_, p);
    HGDIOBJ ob    = SelectObject(dc_, brush);
    ::RoundRect(dc_, r.left, r.top, r.right, r.bottom, corner_w, corner_h);
    SelectObject(dc_, ob);
    SelectObject(dc_, op);
    DeleteObject(brush);
    DeleteObject(p);
}

void Win32Painter::Ellipse(const cerffrontend::Rect& r, cerffrontend::Rgb pen,
                           cerffrontend::Rgb fill) {
    HPEN    p     = CreatePen(PS_SOLID, 1, Colorref(pen));
    HBRUSH  brush = CreateSolidBrush(Colorref(fill));
    HGDIOBJ op    = SelectObject(dc_, p);
    HGDIOBJ ob    = SelectObject(dc_, brush);
    ::Ellipse(dc_, r.left, r.top, r.right, r.bottom);
    SelectObject(dc_, ob);
    SelectObject(dc_, op);
    DeleteObject(brush);
    DeleteObject(p);
}

void Win32Painter::Polyline(const cerffrontend::Point* points, int count, cerffrontend::Rgb pen,
                            int pen_width) {
    if (count < 2) return;
    HPEN    p  = CreatePen(PS_SOLID, pen_width, Colorref(pen));
    HGDIOBJ op = SelectObject(dc_, p);
    MoveToEx(dc_, points[0].x, points[0].y, nullptr);
    for (int i = 1; i < count; ++i) LineTo(dc_, points[i].x, points[i].y);
    SelectObject(dc_, op);
    DeleteObject(p);
}

void Win32Painter::FillCircleAA(int cx, int cy, int radius, cerffrontend::Rgb fill,
                                cerffrontend::Rgb rim) {
    emu_.Get<HostGdiPlus>().FillCircleAA(dc_, cx, cy, radius, Colorref(fill), Colorref(rim));
}

void Win32Painter::FillPolygonAA(const cerffrontend::Point* points, int count,
                                 cerffrontend::Rgb fill, cerffrontend::Rgb rim) {
    const std::vector<POINT> pts = Win32Points(points, count);
    emu_.Get<HostGdiPlus>().FillPolygonAA(dc_, pts.data(), count, Colorref(fill), Colorref(rim));
}

void Win32Painter::FillRoundRectAA(const cerffrontend::Rect& r, int radius, cerffrontend::Rgb fill,
                                   cerffrontend::Rgb rim) {
    emu_.Get<HostGdiPlus>().FillRoundRectAA(dc_, Win32Rect(r), radius, Colorref(fill),
                                            Colorref(rim));
}

void Win32Painter::FillRoundRectAlphaAA(const cerffrontend::Rect& r, int radius,
                                        cerffrontend::Rgb fill, uint8_t alpha) {
    emu_.Get<HostGdiPlus>().FillRoundRectAlphaAA(dc_, Win32Rect(r), radius, Colorref(fill), alpha);
}

void Win32Painter::DrawTextBox(const cerffrontend::Rect& r, std::string_view utf8,
                               const cerffrontend::Font& font, cerffrontend::Rgb color,
                               uint32_t flags) {
    const std::wstring text     = Wide(utf8);
    RECT               rc       = Win32Rect(r);
    const int          old_mode = SetBkMode(dc_, TRANSPARENT);
    const COLORREF     old_clr  = SetTextColor(dc_, Colorref(color));
    HGDIOBJ            old_font = SelectObject(dc_, emu_.Get<Win32PaintResources>().Font(font));
    ::DrawTextW(dc_, text.c_str(), static_cast<int>(text.size()), &rc, DrawTextFlags(flags));
    SelectObject(dc_, old_font);
    SetTextColor(dc_, old_clr);
    SetBkMode(dc_, old_mode);
}

void Win32Painter::DrawTextAt(int x, int y, std::string_view utf8, const cerffrontend::Font& font,
                              cerffrontend::Rgb color) {
    const std::wstring text     = Wide(utf8);
    const int          old_mode = SetBkMode(dc_, TRANSPARENT);
    const COLORREF     old_clr  = SetTextColor(dc_, Colorref(color));
    HGDIOBJ            old_font = SelectObject(dc_, emu_.Get<Win32PaintResources>().Font(font));
    ::TextOutW(dc_, x, y, text.c_str(), static_cast<int>(text.size()));
    SelectObject(dc_, old_font);
    SetTextColor(dc_, old_clr);
    SetBkMode(dc_, old_mode);
}

int Win32Painter::TextHeight(std::string_view utf8, const cerffrontend::Font& font, int width,
                             uint32_t flags) {
    const std::wstring text     = Wide(utf8);
    RECT               rc       = {0, 0, width, 0};
    HGDIOBJ            old_font = SelectObject(dc_, emu_.Get<Win32PaintResources>().Font(font));
    ::DrawTextW(dc_, text.c_str(), static_cast<int>(text.size()), &rc,
                DrawTextFlags(flags) | DT_CALCRECT);
    SelectObject(dc_, old_font);
    return rc.bottom - rc.top;
}

cerffrontend::FontMetrics Win32Painter::Metrics(const cerffrontend::Font& font) {
    HGDIOBJ     old_font = SelectObject(dc_, emu_.Get<Win32PaintResources>().Font(font));
    TEXTMETRICW tm       = {};
    GetTextMetricsW(dc_, &tm);
    SelectObject(dc_, old_font);
    return cerffrontend::FontMetrics{static_cast<int>(tm.tmHeight),
                                     static_cast<int>(tm.tmAveCharWidth)};
}

void Win32Painter::DrawIcon(const cerffrontend::Rect& box, std::string_view stem, int px) {
    emu_.Get<HostIconCache>().DrawSized(dc_, Win32Rect(box), std::string(stem), px);
}

void Win32Painter::DrawImage(const cerffrontend::Rect& dst, cerffrontend::PainterImage image,
                             float opacity) {
    Gdiplus::Bitmap* bmp = emu_.Get<Win32PaintResources>().Image(image);
    if (!bmp || dst.Width() <= 0 || dst.Height() <= 0) return;
    Gdiplus::Graphics g(dc_);
    g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
    Gdiplus::ColorMatrix cm = {};
    cm.m[0][0] = cm.m[1][1] = cm.m[2][2] = 1.0f;
    cm.m[3][3]                           = opacity;
    cm.m[4][4]                           = 1.0f;
    Gdiplus::ImageAttributes ia;
    ia.SetColorMatrix(&cm);
    const Gdiplus::Rect target(dst.left, dst.top, dst.Width(), dst.Height());
    g.DrawImage(bmp, target, 0, 0, static_cast<INT>(bmp->GetWidth()),
                static_cast<INT>(bmp->GetHeight()), Gdiplus::UnitPixel, &ia);
}

bool Win32Painter::ImageSize(cerffrontend::PainterImage image, int& w, int& h) {
    Gdiplus::Bitmap* bmp = emu_.Get<Win32PaintResources>().Image(image);
    if (!bmp) return false;
    w = static_cast<int>(bmp->GetWidth());
    h = static_cast<int>(bmp->GetHeight());
    return true;
}
