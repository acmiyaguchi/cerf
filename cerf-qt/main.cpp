#include <cerffrontend/frontend_launch.h>
#include <cerfcore/core/log.h>
#include "crash_report.h"

#include <QApplication>

int main(int argc, char* argv[]) {
    Log::SetCrashPresenter(&QtCrashReport::Present);
    QApplication app(argc, argv);
    CerfConfig cfg;
    if (const std::optional<int> exit_code = FrontendLaunch::ParseArgs(argc, argv, cfg))
        return *exit_code;
    FrontendLaunch::OpenLog();
    return FrontendLaunch::RunEmulator(cfg, argc, argv);
}
