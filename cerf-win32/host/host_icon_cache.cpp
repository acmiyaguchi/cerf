#define NOMINMAX
#include "host_icon_cache.h"

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/log.h>
#include <cerfcore/platform/wide_text_win32.h>

REGISTER_SERVICE(HostIconCache);

HostIconCache::~HostIconCache() {
    for (auto& e : cache_) DestroyIcon(e.second);
}

void HostIconCache::DrawCentered(HDC dc, const RECT& box,
                                 const std::string& res_name) {
    const int bw = box.right - box.left;
    const int bh = box.bottom - box.top;
    DrawSized(dc, box, res_name, (bw < bh ? bw : bh) - 4);
}

void HostIconCache::DrawSized(HDC dc, const RECT& box, const std::string& res_name, int px) {
    const int bw = box.right - box.left;
    const int bh = box.bottom - box.top;

    HICON h = Resolve(res_name, px);
    if (!h) {
        LOG(Caution, "HostIconCache: ICON resource '%s' absent from cerf.exe "
            "(missing/typo'd cerf.rc entry)\n", res_name.c_str());
        cerfcore::CerfFatalExit(cerfcore::CERF_FATAL_RUNTIME_ERROR);
    }
    DrawIconEx(dc, box.left + (bw - px) / 2, box.top + (bh - px) / 2,
               h, px, px, 0, nullptr, DI_NORMAL);
}

HICON HostIconCache::Resolve(const std::string& res_name, int px) {
    std::pair<std::string, int> key(res_name, px);
    auto it = cache_.find(key);
    if (it != cache_.end()) return it->second;

    /* comctl32's LoadIconWithScaleDown is a NONAME export, so it is reachable
       only by ordinal - and the ordinal is not stable across Windows versions.
       LoadImage scales an icon-group image to cx/cy and is exported by name
       (MS Learn LoadImageW: minimum supported client Windows 2000). */
    HICON h = static_cast<HICON>(LoadImageW(GetModuleHandleW(nullptr),
                                            cerfcore::Utf8ToWide(res_name.c_str()).c_str(),
                                            IMAGE_ICON, px, px,
                                            LR_DEFAULTCOLOR));
    if (h) cache_[key] = h;
    return h;
}
