#include "host_auto_resize.h"

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/device_config.h>
#include <cerfcore/core/user_config_writer.h>
#include <cerfcore/peripherals/cerf_virt/cerf_virt_resize.h>
#include <cerffrontend/guest_additions_ui_policy.h>

REGISTER_SERVICE_AS(HostAutoResize, cerfcore::GuestResizePolicy);

bool HostAutoResize::ShouldRegister() {
    return emu_.Get<cerfcore::DeviceConfig>().guest_additions;
}

void HostAutoResize::OnReady() {
    if (!Available()) enabled_.store(false, std::memory_order_release);
}

bool HostAutoResize::Available() const {
    return emu_.Get<cerffrontend::GuestAdditionsUiPolicy>().LiveResizeAvailable();
}

void HostAutoResize::SetEnabled(bool on) {
    if (!Available()) return;
    enabled_.store(on, std::memory_order_release);
}

void HostAutoResize::OnUserResizeEnd(uint32_t canvas_w, uint32_t canvas_h) {
    if (!Enabled() || canvas_w == 0 || canvas_h == 0) return;
    if (canvas_w == last_w_ && canvas_h == last_h_) return;
    last_w_ = canvas_w;
    last_h_ = canvas_h;
    emu_.Get<cerfcore::CerfVirtResize>().RequestResize(canvas_w, canvas_h);
    emu_.Get<cerfcore::UserConfigWriter>().WriteConfigurableScreenSize(canvas_w, canvas_h);
}
