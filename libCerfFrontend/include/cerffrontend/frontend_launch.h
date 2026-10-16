#pragma once

#include <cerfcore/core/cerf_config.h>

#include <optional>

namespace cerffrontend {

class FrontendLaunch {
public:
    static std::optional<int> ParseArgs(int argc, char** argv, cerfcore::CerfConfig& cfg);
    static void OpenLog();
    static int  RunEmulator(const cerfcore::CerfConfig& cfg, int argc, char** argv);
};

}
