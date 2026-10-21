#pragma once

#include <cerfcore/core/cerf_config.h>

#include <functional>
#include <optional>

namespace cerfcore {
class CerfEmulator;
}

namespace cerffrontend {

class FrontendLaunch {
public:
    using UiLoop = std::function<void(cerfcore::CerfEmulator& emu)>;

    static std::optional<int> ParseArgs(int argc, char** argv, cerfcore::CerfConfig& cfg);
    static void OpenLog();
    static int  RunEmulator(const cerfcore::CerfConfig& cfg, int argc, char** argv,
                            const UiLoop& ui_loop = {});
};

}
