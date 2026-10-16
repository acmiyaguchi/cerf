#pragma once

#include <cerfcore/core/cerf_config.h>

#include <optional>

class FrontendLaunch {
public:
    static std::optional<int> ParseArgs(int argc, char** argv, CerfConfig& cfg);
    static void OpenLog();
    static int  RunEmulator(const CerfConfig& cfg, int argc, char** argv);
};
