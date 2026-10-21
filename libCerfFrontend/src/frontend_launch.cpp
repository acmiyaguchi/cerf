#include <cerffrontend/frontend_launch.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/log.h>
#include <cerffrontend/cli_usage.h>
#include <cerffrontend/main_config.h>
#include <cerffrontend/version.h>

namespace cerffrontend {

std::optional<int> FrontendLaunch::ParseArgs(int argc, char** argv, cerfcore::CerfConfig& cfg) {
    switch (ParseCerfArgs(argc, argv, cfg)) {
        case ArgParseResult::Run:         return std::nullopt;
        case ArgParseResult::BadArgument: return cerfcore::CERF_FATAL_USER_ERROR;
        case ArgParseResult::HelpShown:   break;
    }
    cerfcore::CerfEmulator help(cfg, argc, argv);
    help.CreateAllServices();
    help.Get<CliUsage>().Print(argv[0]);
    return cerfcore::CERF_FATAL_NORMAL_EXIT;
}

void FrontendLaunch::OpenLog() {
    cerfcore::Log::InitDefaultLogFile();
    cerfcore::Log::InstallCrashHandler();
    LOG(Cerf, "== CE Runtime Foundation %s ==\n", CERF_VERSION_DISPLAY_STR);
    LOG(Cerf, "compiled at: %s %s\n", __DATE__, __TIME__);
}

int FrontendLaunch::RunEmulator(const cerfcore::CerfConfig& cfg, int argc, char** argv,
                                const UiLoop& ui_loop) {
    cerfcore::CerfEmulator emu(cfg, argc, argv);
    emu.Boot();
    if (ui_loop) ui_loop(emu);
    emu.WaitForExit();
    cerfcore::Log::Close();
    return 0;
}

}
