#pragma once

#include <cerfcore/core/cerf_config.h>

enum class ArgParseResult {
    Run,
    HelpShown,
    BadArgument,
};

ArgParseResult ParseCerfArgs(int argc, char* argv[], CerfConfig& cfg);
