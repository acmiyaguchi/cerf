#pragma once

#include <cerfcore/core/cerf_config.h>

namespace cerffrontend {

enum class ArgParseResult {
    Run,
    HelpShown,
    BadArgument,
};

ArgParseResult ParseCerfArgs(int argc, char* argv[], cerfcore::CerfConfig& cfg);

}
