#include <cerffrontend/frontend_launch.h>
#include <cerfcore/core/log.h>
#include "crash_report.h"
#include "host_diagnostics.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <timeapi.h>

#include <cerfcore/platform/wide_text_win32.h>

#include <string>
#include <vector>

int wmain(int argc, wchar_t* wargv[]) {
    Log::SetCrashPresenter(&Win32CrashReport::Present);
    std::vector<std::string> utf8_args;
    utf8_args.reserve(argc);
    for (int i = 0; i < argc; ++i) utf8_args.push_back(WideToUtf8(wargv[i]));
    std::vector<char*> argv_ptrs;
    argv_ptrs.reserve(argc + 1);
    for (std::string& a : utf8_args) argv_ptrs.push_back(a.data());
    argv_ptrs.push_back(nullptr);
    char** const argv = argv_ptrs.data();

    CerfConfig cfg;
    if (const std::optional<int> exit_code = FrontendLaunch::ParseArgs(argc, argv, cfg))
        return *exit_code;

    /* Without this, sub-ms cv_.wait_for/Sleep round to the 15.625 ms
       Windows default quantum - OST IRQ latency breaks audio + UI. */
    timeBeginPeriod(1);

    /* dolphin-emu Source/Core/Common/Timer.cpp Timer::IncreaseResolution;
       https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setprocessinformation:
       ControlMask selects both mechanisms, StateMask 0 turns them off -
       "Always honor Timer Resolution Requests" and HighQoS. */
    PROCESS_POWER_THROTTLING_STATE throttling{};
    throttling.Version     = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
    throttling.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED |
                             PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION;
    throttling.StateMask   = 0;
    SetProcessInformation(GetCurrentProcess(), ProcessPowerThrottling,
                          &throttling, sizeof(throttling));

    FrontendLaunch::OpenLog();
    HostDiagnostics::LogReport();

    const int exit_code = FrontendLaunch::RunEmulator(cfg, argc, argv);
    timeEndPeriod(1);
    return exit_code;
}
