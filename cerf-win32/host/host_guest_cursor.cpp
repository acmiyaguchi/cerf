#define NOMINMAX

#include "host_guest_cursor.h"

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/lcd/lcd_pixel_expand.h>
#include <cerfcore/peripherals/cerf_virt/cerf_virt_cursor.h>
#include <cerffrontend/guest_cursor_pixels.h>

#include <vector>

REGISTER_SERVICE(HostGuestCursor);

namespace {

#ifndef CURSOR_CREATION_SCALING_NONE
#define CURSOR_CREATION_SCALING_NONE 1
#endif

/* Disable host cursor scaling for cursors created on this (UI) thread, so the
   guest cursor renders 1:1 - the guest framebuffer is 1:1, so its cursor must
   be too. Resolved dynamically: absent before Win10 1607, where it no-ops and
   the cursor falls back to the host's DPI/accessibility scaling. */
void DisableCursorScaling() {
    typedef UINT (WINAPI *PFN)(UINT);
    static PFN pfn = (PFN)GetProcAddress(GetModuleHandleW(L"user32.dll"),
                                         "SetThreadCursorCreationScaling");
    if (pfn) pfn(CURSOR_CREATION_SCALING_NONE);
}

HCURSOR BuildCursor(const cerfcore::GuestCursorShape& s) {
    const int cx = (int)s.cx, cy = (int)s.cy;
    if (cx <= 0 || cy <= 0) return nullptr;
    const int win_stride = ((cx + 15) / 16) * 2;

    std::vector<uint8_t> andP((size_t)win_stride * cy, 0xFF);
    std::vector<uint8_t> xorP((size_t)win_stride * cy, 0x00);

    const std::vector<cerffrontend::CursorPixel> pixels = cerffrontend::GuestCursorPixels::Decode(s);
    for (int row = 0; row < cy; ++row) {
        for (int col = 0; col < cx; ++col) {
            const uint8_t mask =
                (uint8_t)(1u << cerfcore::lcd_pixel::MsbFirstShift((uint32_t)col, 1u));
            const int didx = row * win_stride + (col >> 3);
            switch (pixels[(size_t)row * cx + col]) {
                case cerffrontend::CursorPixel::kTransparent:
                    break;
                case cerffrontend::CursorPixel::kBlack:
                    andP[didx] &= (uint8_t)~mask;
                    break;
                case cerffrontend::CursorPixel::kWhite:
                    andP[didx] &= (uint8_t)~mask;
                    xorP[didx] |= mask;
                    break;
                case cerffrontend::CursorPixel::kInvert:
                    xorP[didx] |= mask;
                    break;
            }
        }
    }

    DisableCursorScaling();
    return CreateCursor(GetModuleHandleW(nullptr), (int)s.xhot, (int)s.yhot,
                        cx, cy, andP.data(), xorP.data());
}

}  /* namespace */

HostGuestCursor::~HostGuestCursor() {
    if (built_) DestroyCursor(built_);
}

HCURSOR HostGuestCursor::Resolve(bool& active) {
    auto* cur = emu_.TryGet<cerfcore::CerfVirtCursor>();
    if (!cur) { active = false; return nullptr; }

    const uint32_t seq = cur->Seq();
    if (seq != last_seq_) {
        last_seq_ = seq;
        cerfcore::GuestCursorShape s;
        if (cur->GetShape(s)) {
            active_ = true;
            HCURSOR old = built_;
            built_ = s.visible ? BuildCursor(s) : nullptr;
            if (old) DestroyCursor(old);
        }
    }
    active = active_;
    return built_;
}
