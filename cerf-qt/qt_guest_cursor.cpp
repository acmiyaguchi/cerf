#include "qt_guest_cursor.h"

#include <cerffrontend/guest_cursor_pixels.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/peripherals/cerf_virt/cerf_virt_cursor.h>

#include <QImage>
#include <QPixmap>

#include <vector>

REGISTER_SERVICE(QtGuestCursor);

namespace {

constexpr QRgb kBlack = 0xFF000000u;
constexpr QRgb kWhite = 0xFFFFFFFFu;

/* Wine dlls/winex11.drv/mouse.c create_xlib_monochrome_cursor: an inverted pixel is
   drawn black, with a white pixel at (+1, +1). */
QCursor BuildCursor(const cerfcore::GuestCursorShape& s) {
    const int cx = static_cast<int>(s.cx);
    const int cy = static_cast<int>(s.cy);
    if (cx <= 0 || cy <= 0) return QCursor(Qt::BlankCursor);

    const std::vector<cerffrontend::CursorPixel> pixels = cerffrontend::GuestCursorPixels::Decode(s);
    QImage image(cx, cy, QImage::Format_ARGB32);
    image.fill(0u);
    for (int y = 0; y < cy; ++y) {
        for (int x = 0; x < cx; ++x) {
            switch (pixels[static_cast<size_t>(y) * cx + x]) {
                case cerffrontend::CursorPixel::kTransparent: break;
                case cerffrontend::CursorPixel::kBlack:
                case cerffrontend::CursorPixel::kInvert:      image.setPixel(x, y, kBlack); break;
                case cerffrontend::CursorPixel::kWhite:       image.setPixel(x, y, kWhite); break;
            }
        }
    }
    for (int y = 0; y + 1 < cy; ++y)
        for (int x = 0; x + 1 < cx; ++x)
            if (pixels[static_cast<size_t>(y) * cx + x] == cerffrontend::CursorPixel::kInvert)
                image.setPixel(x + 1, y + 1, kWhite);
    return QCursor(QPixmap::fromImage(image), static_cast<int>(s.xhot), static_cast<int>(s.yhot));
}

}

QCursor QtGuestCursor::Resolve(bool& active) {
    auto* cur = emu_.TryGet<cerfcore::CerfVirtCursor>();
    if (!cur) {
        active = false;
        return QCursor();
    }
    const uint32_t seq = cur->Seq();
    if (seq != last_seq_) {
        last_seq_ = seq;
        cerfcore::GuestCursorShape s;
        if (cur->GetShape(s)) {
            active_ = true;
            built_  = s.visible ? BuildCursor(s) : QCursor(Qt::BlankCursor);
        }
    }
    active = active_;
    return built_;
}
