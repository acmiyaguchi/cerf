#define NOMINMAX

#include <cerffrontend/image_exporter.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/log.h>
#include <cerfcore/platform/wide_text_win32.h>

#include <windows.h>
#include <objbase.h>
#include <gdiplus.h>

#include <cstring>
#include <vector>

namespace {

int GetPngEncoderClsid(CLSID& clsid) {
    UINT num = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;
    std::vector<uint8_t> buf(size);
    auto* codecs = reinterpret_cast<Gdiplus::ImageCodecInfo*>(buf.data());
    Gdiplus::GetImageEncoders(num, size, codecs);
    for (UINT i = 0; i < num; ++i) {
        if (wcscmp(codecs[i].MimeType, L"image/png") == 0) {
            clsid = codecs[i].Clsid;
            return (int)i;
        }
    }
    return -1;
}

class Win32ImageExporter : public cerffrontend::ImageExporter {
public:
    using cerffrontend::ImageExporter::ImageExporter;

    bool WritePng(const std::vector<uint32_t>& px, uint32_t w, uint32_t h,
                  const std::string& path) override {
        if (px.empty() || w == 0 || h == 0) return false;
        CLSID png;
        if (GetPngEncoderClsid(png) < 0) {
            LOG(Caution, "Win32ImageExporter: no PNG encoder available\n");
            return false;
        }
        Gdiplus::Bitmap bmp((INT)w, (INT)h, (INT)(w * 4), PixelFormat32bppRGB,
                            reinterpret_cast<BYTE*>(const_cast<uint32_t*>(px.data())));
        const Gdiplus::Status st =
            bmp.Save(cerfcore::Utf8ToWide(path.c_str()).c_str(), &png, nullptr);
        if (st != Gdiplus::Ok) {
            LOG(Caution, "Win32ImageExporter: GDI+ Save failed (status=%d)\n", (int)st);
            return false;
        }
        return true;
    }

    bool CopyToClipboard(const std::vector<uint32_t>& px, uint32_t w, uint32_t h,
                         uint64_t owner) override {
        if (px.empty() || w == 0 || h == 0) return false;

        const size_t row_bytes = (size_t)w * 4u;
        const size_t total     = sizeof(BITMAPINFOHEADER) + row_bytes * h;
        HGLOBAL      hg        = GlobalAlloc(GMEM_MOVEABLE, total);
        if (!hg) {
            LOG(Caution, "Win32ImageExporter::CopyToClipboard: GlobalAlloc failed\n");
            return false;
        }
        auto* base = static_cast<uint8_t*>(GlobalLock(hg));
        auto* bih  = reinterpret_cast<BITMAPINFOHEADER*>(base);
        *bih               = {};
        bih->biSize        = sizeof(BITMAPINFOHEADER);
        bih->biWidth       = (LONG)w;
        bih->biHeight      = (LONG)h;
        bih->biPlanes      = 1;
        bih->biBitCount    = 32;
        bih->biCompression = BI_RGB;
        uint8_t* dst = base + sizeof(BITMAPINFOHEADER);
        for (uint32_t y = 0; y < h; ++y) {
            const uint8_t* src =
                reinterpret_cast<const uint8_t*>(px.data()) + (size_t)(h - 1 - y) * row_bytes;
            memcpy(dst + (size_t)y * row_bytes, src, row_bytes);
        }
        GlobalUnlock(hg);

        if (!OpenClipboard(reinterpret_cast<HWND>(static_cast<uintptr_t>(owner)))) {
            LOG(Caution, "Win32ImageExporter::CopyToClipboard: OpenClipboard failed\n");
            GlobalFree(hg);
            return false;
        }
        EmptyClipboard();
        const bool ok = SetClipboardData(CF_DIB, hg) != nullptr;
        if (!ok) {
            LOG(Caution, "Win32ImageExporter::CopyToClipboard: SetClipboardData failed\n");
            GlobalFree(hg);
        }
        CloseClipboard();
        return ok;
    }
};

}

REGISTER_SERVICE_AS(Win32ImageExporter, cerffrontend::ImageExporter);
