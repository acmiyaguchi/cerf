#include <cerffrontend/frontend_launch.h>
#include <cerfcore/core/log.h>
#include "crash_report.h"

#include <QApplication>

int main(int argc, char* argv[]) {
    cerfcore::Log::SetCrashPresenter(&QtCrashReport::Present);
    QApplication app(argc, argv);
    cerfcore::CerfConfig cfg;
    if (const std::optional<int> exit_code =
            cerffrontend::FrontendLaunch::ParseArgs(argc, argv, cfg))
        return *exit_code;
    cerffrontend::FrontendLaunch::OpenLog();
    return cerffrontend::FrontendLaunch::RunEmulator(cfg, argc, argv);
}
