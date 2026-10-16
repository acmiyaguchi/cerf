#pragma once

#include <cerfcore/host/guest_resize_policy.h>

#include <atomic>
#include <cstdint>

class HostAutoResize : public cerfcore::GuestResizePolicy {
public:
    using cerfcore::GuestResizePolicy::GuestResizePolicy;

    bool ShouldRegister() override;
    void OnReady() override;

    bool Available() const override;
    bool Enabled() const override { return enabled_.load(std::memory_order_acquire); }
    void SetEnabled(bool on) override;

    void OnUserResizeEnd(uint32_t canvas_w, uint32_t canvas_h);

private:
    std::atomic<bool> enabled_{true};
    uint32_t last_w_ = 0;
    uint32_t last_h_ = 0;
};
