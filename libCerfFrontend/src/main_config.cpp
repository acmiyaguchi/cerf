#include <cerffrontend/main_config.h>
#include <cerfcore/core/log.h>
#include <cerfcore/core/run_timeout.h>
#include <cstdlib>
#include <cstring>

namespace cerffrontend {

ArgParseResult ParseCerfArgs(int argc, char* argv[], cerfcore::CerfConfig& cfg) {
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--log=", 6) == 0) {
            cerfcore::Log::SetEnabled(cerfcore::Log::ParseCategories(argv[i] + 6));
        } else if (strncmp(argv[i], "--no-log=", 9) == 0) {
            cfg.no_log_mask |= cerfcore::Log::ParseCategories(argv[i] + 9);
        } else if (strncmp(argv[i], "--log-file=", 11) == 0) {
            cfg.log_file = argv[i] + 11;
        } else if (strcmp(argv[i], "--flush-outputs") == 0) {
            cfg.flush_outputs = true;
        } else if (strncmp(argv[i], "--device=", 9) == 0) {
            cfg.device_override = argv[i] + 9;
        } else if (strncmp(argv[i], "--timeout=", 10) == 0) {
            char* end = nullptr;
            const long secs = strtol(argv[i] + 10, &end, 10);
            if (!end || *end != '\0' || secs <= 0 || secs > 86400) {
                LOG(Caution, "--timeout must be a whole number of seconds in 1..86400 (got '%s')\n", argv[i] + 10);
                return ArgParseResult::BadArgument;
            }
            cfg.timeout_seconds = (int)secs;
        } else if (strcmp(argv[i], "--allow-flood") == 0) {
            cerfcore::Log::SetAllowFlood(true);
        } else if (strncmp(argv[i], cerfcore::kArgBoardId,
                           sizeof(cerfcore::kArgBoardId) - 1) == 0 ||
                   strncmp(argv[i], cerfcore::kArgRomPrimary,
                           sizeof(cerfcore::kArgRomPrimary) - 1) == 0 ||
                   strncmp(argv[i], cerfcore::kArgScreenWidth,
                           sizeof(cerfcore::kArgScreenWidth) - 1) == 0 ||
                   strncmp(argv[i], cerfcore::kArgScreenHeight,
                           sizeof(cerfcore::kArgScreenHeight) - 1) == 0 ||
                   strncmp(argv[i], cerfcore::kArgScreenDpi,
                           sizeof(cerfcore::kArgScreenDpi) - 1) == 0 ||
                   strncmp(argv[i], cerfcore::kArgScreenBpp,
                           sizeof(cerfcore::kArgScreenBpp) - 1) == 0 ||
                   strncmp(argv[i], cerfcore::kArgScreenRefreshRate,
                           sizeof(cerfcore::kArgScreenRefreshRate) - 1) == 0 ||
                   strncmp(argv[i], cerfcore::kArgGaShareFolder,
                           sizeof(cerfcore::kArgGaShareFolder) - 1) == 0 ||
                   strncmp(argv[i], cerfcore::kArgUsbSerialHost,
                           sizeof(cerfcore::kArgUsbSerialHost) - 1) == 0 ||
                   strncmp(argv[i], cerfcore::kArgBoot, sizeof(cerfcore::kArgBoot) - 1) == 0 ||
                   strncmp(argv[i], cerfcore::kArgTab, sizeof(cerfcore::kArgTab) - 1) == 0 ||
                   strncmp(argv[i], cerfcore::kArgGaColorScheme,
                           sizeof(cerfcore::kArgGaColorScheme) - 1) == 0 ||
                   strncmp(argv[i], cerfcore::kArgGaFontSize,
                           sizeof(cerfcore::kArgGaFontSize) - 1) == 0 ||
                   strncmp(argv[i], cerfcore::kArgGaAutorun,
                           sizeof(cerfcore::kArgGaAutorun) - 1) == 0 ||
                   strcmp(argv[i], cerfcore::kArgDisableNetwork) == 0 ||
                   strcmp(argv[i], cerfcore::kArgGuestAdditions) == 0 ||
                   strcmp(argv[i], cerfcore::kArgFullScreen) == 0 ||
                   strcmp(argv[i], cerfcore::kArgNoFocus) == 0 ||
                   strcmp(argv[i], cerfcore::kArgRecovery) == 0) {
            /* Device-config overrides - applied to DeviceConfig by
               ConfigLoader after cerf.json loads. Recognized here only so
               they are not rejected as unknown args. */
        } else if (strcmp(argv[i], "--quiet") == 0) {
            cerfcore::Log::SetEnabled(cerfcore::Log::MASK_NONE);
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            return ArgParseResult::HelpShown;
        } else {
            LOG(Caution, "Unknown argument: %s (use --help)\n", argv[i]);
            return ArgParseResult::BadArgument;
        }
    }

    if (cfg.no_log_mask) {
        cerfcore::Log::SetEnabled(cerfcore::Log::GetEnabled() & ~cfg.no_log_mask);
    }

    if (cfg.flush_outputs) {
        cerfcore::Log::SetFlush(true);
    }

    if (cfg.log_file) {
        cerfcore::Log::SetFile(cfg.log_file);
    }

    cerfcore::RunTimeout::Start(cfg.timeout_seconds);

    return ArgParseResult::Run;
}

}
