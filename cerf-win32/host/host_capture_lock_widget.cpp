#define NOMINMAX
#include <windows.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/service.h>
#include <cerfcore/platform/wide_text_win32.h>
#include "host_input_capture.h"
#include "host_key_binding.h"
#include <cerfcore/host/host_widget.h>
#include <cerfcore/host/host_widget_registry.h>

#include <string>
#include <vector>

namespace {

class HostCaptureLockWidget : public Service, public HostWidget {
public:
    using Service::Service;

    void OnReady() override {
        emu_.Get<HostWidgetRegistry>().Register(this);
    }

    std::string WidgetName() const override { return "Input Capture"; }
    WidgetGroup Group() const override { return WidgetGroup::InputCapture; }
    std::string Tooltip() const override {
        const std::string key = WideToUtf8(emu_.Get<HostKeyBinding>().Label());
        return emu_.Get<HostInputCapture>().IsCaptured()
            ? "Input captured - " + key + " (or click) to release"
            : "Input free - " + key +
                  " (or click) to capture (Alt+Tab etc. -> guest)";
    }
    bool TryPrimaryAction() override {
        emu_.Get<HostInputCapture>().Toggle();
        return true;
    }
    std::vector<WidgetControl> Controls() override {
        const std::string key = WideToUtf8(emu_.Get<HostKeyBinding>().Label());
        ActionControl it;
        it.id    = "toggle_capture";
        it.label = emu_.Get<HostInputCapture>().IsCaptured()
                       ? "Release input capture\t" + key
                       : "Capture input\t" + key;
        it.press = [this] { emu_.Get<HostInputCapture>().Toggle(); };
        return {WidgetControl{std::move(it)}};
    }
    WidgetIcon Icon() const override {
        const bool cap = emu_.Get<HostInputCapture>().IsCaptured();
        return ResourceIcon{cap ? "ICON_LOCK_ON" : "ICON_LOCK_OFF"};
    }
    bool PollDirty() override {
        const bool cap = emu_.Get<HostInputCapture>().IsCaptured();
        if (cap == last_drawn_cap_) return false;
        last_drawn_cap_ = cap;
        return true;
    }

private:
    bool last_drawn_cap_ = false;
};

}  /* namespace */

REGISTER_SERVICE(HostCaptureLockWidget);
