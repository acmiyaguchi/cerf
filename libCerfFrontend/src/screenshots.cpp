#include <cerffrontend/screenshots.h>

#include <cerffrontend/frontend_window.h>
#include <cerffrontend/image_exporter.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/cerf_paths.h>
#include <cerfcore/core/device_config.h>
#include <cerfcore/core/log.h>
#include <cerfcore/core/utf16.h>
#include <cerfcore/platform/host_local_time.h>

#include <cstdio>
#include <ctime>
#include <filesystem>
#include <system_error>

namespace cerffrontend {

REGISTER_SERVICE(Screenshots);

namespace {

std::string SanitizeForFilename(std::string name) {
    if (name.empty()) name = "device";
    for (char& c : name) {
        if (c == '\\' || c == '/' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' ||
            c == '>' || c == '|')
            c = '_';
    }
    return name;
}

std::string TimestampNow() {
    std::tm local{};
    cerfcore::host_platform::LocalCalendarTime(std::time(nullptr), local);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d%02d%02d_%02d%02d%02d", local.tm_year + 1900,
                  local.tm_mon + 1, local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);
    return buf;
}

}

void Screenshots::Save() {
    std::vector<uint32_t> px;
    uint32_t              w = 0, h = 0;
    if (!emu_.Get<FrontendWindow>().CaptureGuestSurface(px, w, h)) {
        LOG(Lcd, "Screenshots::Save: no guest frame to capture\n");
        return;
    }
    SavePixels(px, w, h, emu_.Get<cerfcore::DeviceConfig>().device_name);
}

void Screenshots::Copy() {
    std::vector<uint32_t> px;
    uint32_t              w = 0, h = 0;
    if (!emu_.Get<FrontendWindow>().CaptureGuestSurface(px, w, h)) {
        LOG(Lcd, "Screenshots::Copy: no guest frame to capture\n");
        return;
    }
    CopyPixels(px, w, h, emu_.Get<FrontendWindow>().NativeHandle());
}

void Screenshots::CopyPixels(const std::vector<uint32_t>& pixels, uint32_t w, uint32_t h,
                             uint64_t owner) {
    if (pixels.empty() || w == 0 || h == 0) return;
    if (emu_.Get<ImageExporter>().CopyToClipboard(pixels, w, h, owner))
        LOG(Lcd, "Screenshots::CopyPixels: copied %ux%u image\n", w, h);
}

void Screenshots::SaveGuestSurfaceTo(const std::string& path) {
    std::vector<uint32_t> px;
    uint32_t              w = 0, h = 0;
    if (!emu_.Get<FrontendWindow>().CaptureGuestSurface(px, w, h)) {
        LOG(Lcd, "Screenshots::SaveGuestSurfaceTo: no guest frame to capture\n");
        return;
    }
    if (emu_.Get<ImageExporter>().WritePng(px, w, h, path))
        LOG(Lcd, "Screenshots::SaveGuestSurfaceTo: wrote %ux%u\n", w, h);
}

void Screenshots::SavePixels(const std::vector<uint32_t>& pixels, uint32_t w, uint32_t h,
                             const std::string& name_hint) {
    if (pixels.empty() || w == 0 || h == 0) return;
    const std::filesystem::path dir =
        std::filesystem::path(cerfcore::Utf8ToUtf16(cerfcore::GetCerfDir())) / u"screenshots";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    const std::filesystem::path file =
        dir / cerfcore::Utf8ToUtf16(SanitizeForFilename(name_hint) + "_" + TimestampNow() + ".png");
    if (emu_.Get<ImageExporter>().WritePng(pixels, w, h, cerfcore::Utf16ToUtf8(file.u16string())))
        LOG(Lcd, "Screenshots::SavePixels: wrote %ux%u screenshot\n", w, h);
}

}
