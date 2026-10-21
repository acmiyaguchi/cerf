#include <cerfcore/host/shutdown_host.h>

#include "qt_main_window.h"
#include "qt_shutdown_dialog.h"

#include <cerfcore/core/cerf_emulator.h>
#include <cerffrontend/shutdown_prompt.h>

#include <QCoreApplication>
#include <QMetaObject>

namespace {

class QtShutdownHost : public cerfcore::ShutdownHost {
public:
    using cerfcore::ShutdownHost::ShutdownHost;

    void RunPromptJob(std::function<void()> job) override {
        QMetaObject::invokeMethod(QCoreApplication::instance(), std::move(job),
                                  Qt::QueuedConnection);
    }

    cerfcore::ShutdownChoice Ask(cerfcore::ShutdownTrigger trigger) override {
        return emu_.Get<QtShutdownDialog>().Show(trigger);
    }

    void DismissAsCancel() override { emu_.Get<QtShutdownDialog>().DismissAsCancel(); }

    void BeginTeardown() override { emu_.Get<QtMainWindow>().BeginShutdownTeardown(); }

    void CancelTeardown() override {
        emu_.Get<cerffrontend::ShutdownPrompt>().CancelCloseRequest();
    }
};

}

REGISTER_SERVICE_AS(QtShutdownHost, cerfcore::ShutdownHost);
