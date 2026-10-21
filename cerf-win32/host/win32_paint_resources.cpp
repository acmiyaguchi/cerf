#define NOMINMAX
#include "win32_paint_resources.h"

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/fatal.h>
#include "host_dark_mode.h"
#include "host_fonts.h"
#include "host_gdiplus.h"

#include <objbase.h>
#include <gdiplus.h>

REGISTER_SERVICE(Win32PaintResources);

Win32PaintResources::~Win32PaintResources() {
    for (auto& entry : fonts_) DeleteObject(entry.second);
}

void Win32PaintResources::OnShutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    delete logo_;
    logo_ = nullptr;
}

HFONT Win32PaintResources::Font(const cerffrontend::Font& font) {
    if (font.face == cerffrontend::FontFace::kSystem) {
        HFONT ui = emu_.Get<HostDarkMode>().UiFont();
        if (!ui) ui = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        if (font.px == 0 && !font.bold) return ui;
        std::lock_guard<std::mutex> lock(mutex_);
        const auto key = std::make_tuple(static_cast<uint8_t>(font.face), font.px, font.bold);
        if (auto it = fonts_.find(key); it != fonts_.end()) return it->second;
        LOGFONTW lf = {};
        if (GetObjectW(ui, sizeof(lf), &lf) != sizeof(lf)) return ui;
        if (lf.lfHeight < 0)      lf.lfHeight -= font.px;
        else if (lf.lfHeight > 0) lf.lfHeight += font.px;
        if (font.bold) lf.lfWeight = FW_BOLD;
        const HFONT derived = CreateFontIndirectW(&lf);
        if (!derived)
            emu_.Get<cerfcore::Fatal>().Die("Win32PaintResources: system font %+dpx failed (gle=%lu)",
                                            font.px, GetLastError());
        fonts_.emplace(key, derived);
        return derived;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    const auto key = std::make_tuple(static_cast<uint8_t>(font.face), font.px, font.bold);
    auto       it  = fonts_.find(key);
    if (it != fonts_.end()) return it->second;

    HostFonts&  fonts = emu_.Get<HostFonts>();
    const bool  mono  = font.face == cerffrontend::FontFace::kMono;
    const HFONT h = CreateFontW(-font.px, 0, 0, 0, font.bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE,
                                FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY,
                                mono ? (FIXED_PITCH | FF_MODERN) : (VARIABLE_PITCH | FF_SWISS),
                                mono ? fonts.MonoFace() : fonts.UiFace());
    if (!h)
        emu_.Get<cerfcore::Fatal>().Die("Win32PaintResources: CreateFontW %dpx failed (gle=%lu)",
                                        font.px, GetLastError());
    fonts_.emplace(key, h);
    return h;
}

Gdiplus::Bitmap* Win32PaintResources::Image(cerffrontend::PainterImage image) {
    std::lock_guard<std::mutex> lock(mutex_);
    switch (image) {
        case cerffrontend::PainterImage::kCerfLogo:
            if (!logo_loaded_) {
                logo_loaded_ = true;
                logo_        = emu_.Get<HostGdiPlus>().DecodeResourcePng(L"CERF_LOGO");
            }
            return logo_;
    }
    return nullptr;
}
