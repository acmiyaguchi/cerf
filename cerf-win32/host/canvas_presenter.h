#pragma once

#define NOMINMAX
#include <windows.h>

#include <cerfcore/host/frame_source.h>
#include <cerffrontend/viewport_layout.h>

#include <cstdint>
#include <vector>

class CanvasPresenter {
public:
    explicit CanvasPresenter(cerfcore::FrameSource* source) : source_(source) {}
    ~CanvasPresenter();

    void SetSource(cerfcore::FrameSource* source) { source_ = source; }
    bool SourceHasFrame() const { return source_ && source_->HasFrame(); }

    CanvasPresenter(const CanvasPresenter&)            = delete;
    CanvasPresenter& operator=(const CanvasPresenter&) = delete;

    void Attach(HWND hwnd, uint32_t surf_w, uint32_t surf_h);
    void Detach();

    void OnCanvasResized(int canvas_w, int canvas_h);
    void SetSurfaceSize(uint32_t w, uint32_t h);

    uint32_t SurfaceWidth() const { return layout_.SurfaceWidth(); }
    uint32_t SurfaceHeight() const { return layout_.SurfaceHeight(); }

    int ContentWidth() const { return layout_.ContentWidth(); }
    int ContentHeight() const { return layout_.ContentHeight(); }

    cerffrontend::ViewportMode Mode() const { return layout_.Mode(); }
    bool                       Antialias() const { return layout_.Antialias(); }
    int                        IntegerFactor() const { return layout_.IntegerFactor(); }
    void                       SetViewportMode(cerffrontend::ViewportMode m);
    void                       SetAntialias(bool on);
    void                       SetIntegerScale(int factor);

    bool ComposeInto(HDC present_dc, uint32_t* present_bits);

    void SetActive(bool active);

    void UpdateScrollbars();
    void OnHScroll(WPARAM wp);
    void OnVScroll(WPARAM wp);

    bool HostToGuest(int cx, int cy, int& sx, int& sy) const {
        return layout_.HostToGuest(cx, cy, sx, sy);
    }
    void ClampGuest(int& sx, int& sy) const { layout_.ClampGuest(sx, sy); }

    bool CaptureSurface(std::vector<uint32_t>& out, uint32_t& w, uint32_t& h);

private:
    void RebuildGuestDib(uint32_t w, uint32_t h);
    void OnScroll(bool horizontal, WPARAM wp);
    void Invalidate() const;

    cerfcore::FrameSource*     source_;
    cerffrontend::ViewportLayout layout_;

    HWND      hwnd_       = nullptr;
    HDC       guest_dc_   = nullptr;
    HBITMAP   guest_dib_  = nullptr;
    uint32_t* guest_bits_ = nullptr;

    bool hsb_shown_ = false;
    bool vsb_shown_ = false;
};
