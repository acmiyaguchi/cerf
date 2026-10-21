#include <cerffrontend/host_key_binding.h>

#include <cerffrontend/frontend_config.h>
#include <cerffrontend/host_virtual_keys.h>

#include <cerfcore/core/cerf_emulator.h>

namespace cerffrontend {

void HostKeyBinding::OnReady() {
    vks_ = emu_.Get<FrontendConfig>().HostKeyVks();
    if (vks_.empty()) vks_.push_back(kVkRControl);
    for (size_t i = 0; i < vks_.size(); i++) {
        if (i) label_ += '+';
        label_ += KeyName(vks_[i]);
    }
}

int HostKeyBinding::IndexOf(uint32_t vk) const {
    for (size_t i = 0; i < vks_.size(); i++)
        if (vks_[i] == vk) return static_cast<int>(i);
    return -1;
}

}
