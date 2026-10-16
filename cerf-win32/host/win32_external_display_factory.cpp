#include <cerfcore/host/external_display.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/platform/wide_text_win32.h>
#include "external_display_window.h"

namespace {

class Win32ExternalDisplayFactory : public ExternalDisplayFactory {
public:
    using ExternalDisplayFactory::ExternalDisplayFactory;

    std::unique_ptr<ExternalDisplay> Create(FrameSource& source, const std::string& title,
                                            std::function<void()> on_eject) override {
        return std::make_unique<ExternalDisplayWindow>(emu_, source,
                                                       Utf8ToWide(title.c_str()),
                                                       std::move(on_eject));
    }
};

}

REGISTER_SERVICE_AS(Win32ExternalDisplayFactory, ExternalDisplayFactory);
