#define NOMINMAX

#include "canvas_presenter.h"

#include <cerfcore/core/log.h>
#include <cerfcore/host/frame_source.h>

#include <cstring>

CanvasPresenter::~CanvasPresenter() { Detach(); }

void CanvasPresenter::Attach(HWND hwnd, uint32_t surf_w, uint32_t surf_h) {
    hwnd_ = hwnd;
    RebuildGuestDib(surf_w, surf_h);
}

void CanvasPresenter::Detach() {
    if (guest_dc_)  { DeleteDC(guest_dc_); guest_dc_ = nullptr; }
    if (guest_dib_) { DeleteObject(guest_dib_); guest_dib_ = nullptr; guest_bits_ = nullptr; }
    hwnd_ = nullptr;
}

void CanvasPresenter::Invalidate() const {
    if (hwnd_) InvalidateRect(hwnd_, nullptr, FALSE);
}

void CanvasPresenter::RebuildGuestDib(uint32_t w, uint32_t h) {
    if (w == 0) w = 1;
    if (h == 0) h = 1;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize        = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth       = (LONG)w;
    bmi.bmiHeader.biHeight      = -(LONG)h;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC screen = GetDC(hwnd_);
    void* bits = nullptr;
    HBITMAP nd = CreateDIBSection(screen, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    ReleaseDC(hwnd_, screen);
    if (!nd || !bits) {
        LOG(Caution, "CanvasPresenter::RebuildGuestDib %ux%u failed (gle=%lu)\n",
            w, h, GetLastError());
        if (nd) DeleteObject(nd);
        return;
    }
    if (!guest_dc_) guest_dc_ = CreateCompatibleDC(nullptr);
    SelectObject(guest_dc_, nd);
    if (guest_dib_) DeleteObject(guest_dib_);
    guest_dib_  = nd;
    guest_bits_ = static_cast<uint32_t*>(bits);
    layout_.SetSurfaceSize(w, h);
}

void CanvasPresenter::OnCanvasResized(int canvas_w, int canvas_h) {
    layout_.SetCanvasSize(canvas_w, canvas_h);
    UpdateScrollbars();
}

void CanvasPresenter::SetSurfaceSize(uint32_t w, uint32_t h) {
    if (w == layout_.SurfaceWidth() && h == layout_.SurfaceHeight()) return;
    RebuildGuestDib(w, h);
    layout_.ResetScroll();
    UpdateScrollbars();
    Invalidate();
}

void CanvasPresenter::SetActive(bool active) {
    if (!layout_.SetActive(active)) return;
    UpdateScrollbars();
    Invalidate();
}

void CanvasPresenter::SetViewportMode(cerffrontend::ViewportMode m) {
    if (!layout_.SetMode(m)) return;
    UpdateScrollbars();
    Invalidate();
}

void CanvasPresenter::SetAntialias(bool on) {
    if (layout_.SetAntialias(on)) Invalidate();
}

void CanvasPresenter::SetIntegerScale(int factor) {
    if (!layout_.SetIntegerScale(factor)) return;
    UpdateScrollbars();
    Invalidate();
}

bool CanvasPresenter::ComposeInto(HDC present_dc, uint32_t* present_bits) {
    const int cw = layout_.CanvasWidth();
    const int ch = layout_.CanvasHeight();
    if (!present_bits || cw <= 0 || ch <= 0) return false;

    if (!source_ || !source_->HasFrame() || !guest_bits_) {
        std::memset(present_bits, 0, (size_t)cw * ch * 4u);
        return false;
    }
    source_->RenderInto(guest_bits_, SurfaceWidth(), SurfaceHeight());

    std::memset(present_bits, 0, (size_t)cw * ch * 4u);
    const cerffrontend::ViewportLayout::Layout L = layout_.Compute();
    if (L.dst_w <= 0 || L.dst_h <= 0) return true;
    if (L.stretch) {
        SetStretchBltMode(present_dc, layout_.Antialias() ? HALFTONE : COLORONCOLOR);
        SetBrushOrgEx(present_dc, 0, 0, nullptr);
        StretchBlt(present_dc, L.dst_x, L.dst_y, L.dst_w, L.dst_h,
                   guest_dc_, L.src_x, L.src_y, L.src_w, L.src_h, SRCCOPY);
    } else {
        BitBlt(present_dc, L.dst_x, L.dst_y, L.dst_w, L.dst_h,
               guest_dc_, L.src_x, L.src_y, SRCCOPY);
    }
    return true;
}

void CanvasPresenter::UpdateScrollbars() {
    if (!hwnd_) return;
    const cerffrontend::ViewportLayout::Scroll s = layout_.UpdateScroll();

    if (s.horizontal != hsb_shown_) { ShowScrollBar(hwnd_, SB_HORZ, s.horizontal); hsb_shown_ = s.horizontal; }
    if (s.vertical != vsb_shown_)   { ShowScrollBar(hwnd_, SB_VERT, s.vertical);   vsb_shown_ = s.vertical; }

    if (s.horizontal) {
        SCROLLINFO si = { sizeof(si) };
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMin = 0; si.nMax = s.content_w - 1; si.nPage = (UINT)s.page_w; si.nPos = s.pos_x;
        SetScrollInfo(hwnd_, SB_HORZ, &si, TRUE);
    }
    if (s.vertical) {
        SCROLLINFO si = { sizeof(si) };
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMin = 0; si.nMax = s.content_h - 1; si.nPage = (UINT)s.page_h; si.nPos = s.pos_y;
        SetScrollInfo(hwnd_, SB_VERT, &si, TRUE);
    }
}

void CanvasPresenter::OnScroll(bool horizontal, WPARAM wp) {
    if (!hwnd_) return;
    const int  bar = horizontal ? SB_HORZ : SB_VERT;
    SCROLLINFO si  = { sizeof(si) };
    si.fMask = SIF_ALL;
    GetScrollInfo(hwnd_, bar, &si);
    int pos = si.nPos;
    switch (LOWORD(wp)) {
        case SB_LINEUP:        pos -= cerffrontend::ViewportLayout::kScrollLineStep; break;
        case SB_LINEDOWN:      pos += cerffrontend::ViewportLayout::kScrollLineStep; break;
        case SB_PAGEUP:        pos -= (int)si.nPage;                                 break;
        case SB_PAGEDOWN:      pos += (int)si.nPage;                                 break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION: pos = si.nTrackPos;                                   break;
    }
    layout_.ScrollTo(horizontal, pos);
    SCROLLINFO s2 = { sizeof(s2) };
    s2.fMask = SIF_POS;
    s2.nPos  = layout_.ScrollPos(horizontal);
    SetScrollInfo(hwnd_, bar, &s2, TRUE);
    Invalidate();
}

void CanvasPresenter::OnHScroll(WPARAM wp) { OnScroll(true, wp); }

void CanvasPresenter::OnVScroll(WPARAM wp) { OnScroll(false, wp); }

bool CanvasPresenter::CaptureSurface(std::vector<uint32_t>& out, uint32_t& w, uint32_t& h) {
    if (!source_ || !source_->HasFrame()) return false;
    const uint32_t sw = SurfaceWidth();
    const uint32_t sh = SurfaceHeight();
    if (sw == 0 || sh == 0 || !guest_bits_) return false;
    source_->RenderInto(guest_bits_, sw, sh);
    out.assign(guest_bits_, guest_bits_ + (size_t)sw * sh);
    w = sw;
    h = sh;
    return true;
}
