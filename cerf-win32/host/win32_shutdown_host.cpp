#include <cerfcore/host/shutdown_host.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerffrontend/shutdown_prompt.h>
#include "shutdown_dialog.h"
#include "host_window.h"

namespace {

class Win32ShutdownHost : public cerfcore::ShutdownHost {
public:
    using cerfcore::ShutdownHost::ShutdownHost;

    void RunPromptJob(std::function<void()> job) override {
        emu_.Get<HostWindow>().RunOnUiThread(std::move(job));
    }

    cerfcore::ShutdownChoice Ask(cerfcore::ShutdownTrigger trigger) override {
        return emu_.Get<ShutdownDialog>().Show(trigger);
    }

    void DismissAsCancel() override { emu_.Get<ShutdownDialog>().DismissAsCancel(); }

    void BeginTeardown() override { emu_.Get<HostWindow>().BeginShutdownTeardown(); }
    void CancelTeardown() override {
        emu_.Get<cerffrontend::ShutdownPrompt>().CancelCloseRequest();
    }
};

}

REGISTER_SERVICE_AS(Win32ShutdownHost, cerfcore::ShutdownHost);
