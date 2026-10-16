#pragma once

#include <cerfcore/core/service.h>

#define NOMINMAX
#include <windows.h>

#include <cstdint>
#include <string>
#include <vector>

class HwScreenRenderer : public Service {
public:
    using Service::Service;
    ~HwScreenRenderer() override;

    void RenderInto(HDC dc, uint32_t* dib_bgra32, uint32_t width, uint32_t height);

private:
    HFONT font_cache_[2] = { nullptr, nullptr };

    HDC       bg_dc_    = nullptr;
    HBITMAP   bg_dib_   = nullptr;
    uint32_t* bg_bits_  = nullptr;
    int       bg_w_     = 0;
    int       bg_h_     = 0;
    uint64_t  bg_gen_   = ~0ull;
    bool      bg_valid_ = false;

    void RebuildBgDib(int w, int h);
    void DrawLog(HDC dc, uint32_t width, uint32_t height,
                 const std::vector<std::string>& lines);
};
