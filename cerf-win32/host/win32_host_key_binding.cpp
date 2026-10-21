#define NOMINMAX

#include <cerffrontend/host_key_binding.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/platform/wide_text_win32.h>

#include <windows.h>

#include <cstdio>

namespace {

constexpr uint32_t kExtendedKeyLParamBit = 1u << 24;

bool IsExtendedVk(uint8_t vk) {
    switch (vk) {
        case VK_RMENU:
        case VK_RCONTROL:
        case VK_INSERT:
        case VK_DELETE:
        case VK_HOME:
        case VK_END:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_LEFT:
        case VK_RIGHT:
        case VK_UP:
        case VK_DOWN:
        case VK_CANCEL:
        case VK_SNAPSHOT:
        case VK_DIVIDE:
        case VK_LWIN:
        case VK_RWIN:
        case VK_APPS:
            return true;
        default:
            return false;
    }
}

class Win32HostKeyBinding : public cerffrontend::HostKeyBinding {
public:
    using cerffrontend::HostKeyBinding::HostKeyBinding;

    bool AllMembersDownNow() const override {
        for (uint8_t vk : Vks())
            if (!(GetKeyState(vk) & 0x8000)) return false;
        return true;
    }

protected:
    std::string KeyName(uint8_t vk) const override {
        const UINT sc = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
        if (sc) {
            LONG lparam = (LONG)(sc << 16);
            if (IsExtendedVk(vk)) lparam |= (LONG)kExtendedKeyLParamBit;
            wchar_t name[64] = {};
            if (GetKeyNameTextW(lparam, name, 64) > 0 && name[0] >= L' ')
                return cerfcore::WideToUtf8(name);
        }
        char fallback[16] = {};
        std::snprintf(fallback, sizeof(fallback), "VK 0x%02X", (unsigned)vk);
        return fallback;
    }
};

}

REGISTER_SERVICE_AS(Win32HostKeyBinding, cerffrontend::HostKeyBinding);
