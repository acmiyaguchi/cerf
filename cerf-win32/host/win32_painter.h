#pragma once

#include <cerffrontend/painter.h>

#define NOMINMAX
#include <windows.h>

namespace cerfcore { class CerfEmulator; }

class Win32Painter final : public cerffrontend::Painter {
public:
    Win32Painter(cerfcore::CerfEmulator& emu, HDC dc) : emu_(emu), dc_(dc) {}

    void FillRect(const cerffrontend::Rect& r, cerffrontend::Rgb color) override;
    void BlendRect(const cerffrontend::Rect& r, cerffrontend::Rgb color, uint8_t alpha) override;
    void Rectangle(const cerffrontend::Rect& r, cerffrontend::Rgb pen, int pen_width,
                   const cerffrontend::Rgb* fill) override;
    void RoundRect(const cerffrontend::Rect& r, int corner_w, int corner_h, cerffrontend::Rgb pen,
                   cerffrontend::Rgb fill) override;
    void Ellipse(const cerffrontend::Rect& r, cerffrontend::Rgb pen, cerffrontend::Rgb fill) override;
    void Polyline(const cerffrontend::Point* points, int count, cerffrontend::Rgb pen,
                  int pen_width) override;

    void FillCircleAA(int cx, int cy, int radius, cerffrontend::Rgb fill,
                      cerffrontend::Rgb rim) override;
    void FillPolygonAA(const cerffrontend::Point* points, int count, cerffrontend::Rgb fill,
                       cerffrontend::Rgb rim) override;
    void FillRoundRectAA(const cerffrontend::Rect& r, int radius, cerffrontend::Rgb fill,
                         cerffrontend::Rgb rim) override;
    void FillRoundRectAlphaAA(const cerffrontend::Rect& r, int radius, cerffrontend::Rgb fill,
                              uint8_t alpha) override;

    void DrawTextBox(const cerffrontend::Rect& r, std::string_view utf8,
                     const cerffrontend::Font& font, cerffrontend::Rgb color,
                     uint32_t flags) override;
    void DrawTextAt(int x, int y, std::string_view utf8, const cerffrontend::Font& font,
                    cerffrontend::Rgb color) override;
    int  TextHeight(std::string_view utf8, const cerffrontend::Font& font, int width,
                    uint32_t flags) override;
    cerffrontend::FontMetrics Metrics(const cerffrontend::Font& font) override;

    void DrawIcon(const cerffrontend::Rect& box, std::string_view stem, int px) override;
    void DrawImage(const cerffrontend::Rect& dst, cerffrontend::PainterImage image,
                   float opacity) override;
    bool ImageSize(cerffrontend::PainterImage image, int& w, int& h) override;

private:
    cerfcore::CerfEmulator& emu_;
    HDC                     dc_;
};
