#include <cerffrontend/launcher_transaction.h>

#include <cerffrontend/launcher_process.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/cerf_paths.h>
#include <cerfcore/core/device_config.h>
#include <cerfcore/core/log.h>
#include <cerfcore/core/utf16.h>
#include <cerfcore/host/user_alert.h>
#include <cerfcore/platform/host_file.h>
#include <cerfcore/platform/host_process.h>

#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <vector>

namespace cerffrontend {

REGISTER_SERVICE(LauncherTransaction);

namespace {

constexpr char kTitle[] = "CERF configuration";

std::filesystem::path FsPath(const std::string& utf8) {
    return std::filesystem::path(cerfcore::Utf8ToUtf16(utf8));
}

}

void LauncherTransaction::Complain(const std::string& text) {
    LOG(Caution, "LauncherTransaction: %s\n", text.c_str());
    emu_.Get<cerfcore::UserAlert>().Show(cerfcore::AlertLevel::kError,
                                         cerfcore::AlertPlacement::kOverWindow, kTitle, text);
}

bool LauncherTransaction::LocateLauncher(std::string& exe) {
    exe = emu_.Get<LauncherProcess>().ExecutablePath();
    if (cerfcore::host_platform::HostFileExists(exe)) return true;
    Complain("The launcher is missing, so the configuration window cannot open.\n\n" + exe);
    return false;
}

bool LauncherTransaction::WriteRequest(const std::string& path, const nlohmann::json& request) {
    std::ofstream f(FsPath(path), std::ios::trunc | std::ios::binary);
    if (f.is_open()) {
        f << request.dump(2) << '\n';
        if (f.good()) return true;
    }
    Complain("CERF could not write its request for the configuration window.\n\n" + path);
    return false;
}

bool LauncherTransaction::Spawn(uint64_t owner, const std::string& exe, const std::string& device,
                                const std::string& file) {
    const std::vector<std::string> args = {"transactional", device, file, std::to_string(owner)};
    uint32_t                       exit_code = 0;
    std::string                    error;
    if (!emu_.Get<LauncherProcess>().RunAndWait(owner, args, exit_code, error)) {
        Complain("CERF could not start the configuration window (" + error + ").\n\n" + exe);
        return false;
    }
    if (exit_code == 0) return true;
    Complain("The configuration window closed with an error (exit code " +
             std::to_string(exit_code) + ").");
    return false;
}

bool LauncherTransaction::ReadResponse(const std::string& path, const char* key,
                                       nlohmann::json& response) {
    nlohmann::json parsed;
    {
        std::ifstream f(FsPath(path));
        if (!f.is_open()) {
            Complain("The configuration window left no answer behind.\n\n" + path);
            return false;
        }
        try {
            f >> parsed;
        } catch (const std::exception&) {
            Complain("CERF could not read the answer from the configuration window.\n\n" + path);
            return false;
        }
    }
    if (!parsed.is_object() || !parsed.contains(key)) return true;
    const auto& block = parsed[key];
    if (!block.is_object() || !block.contains("response")) return true;
    const auto& answer = block["response"];
    if (answer.is_object()) response = answer;
    return true;
}

bool LauncherTransaction::Run(uint64_t owner, const char* scaffolding, const nlohmann::json& query,
                              nlohmann::json& response) {
    if (running_) return false;
    running_ = true;

    std::string exe;
    bool        ok = LocateLauncher(exe);
    if (ok) {
        nlohmann::json request;
        request[scaffolding]["query"] = query;

        const std::string device = emu_.Get<cerfcore::DeviceConfig>().device_name;
        char              file[64];
        std::snprintf(file, sizeof(file), "transactional-%08X-%04X.json",
                      cerfcore::host_platform::CurrentProcessId(), ++serial_);
        const std::string path = cerfcore::GetDeviceDir(device) + file;

        ok = WriteRequest(path, request);
        if (ok) {
            ok = Spawn(owner, exe, device, file);
            if (ok) ok = ReadResponse(path, scaffolding, response);
            uint32_t error = 0;
            cerfcore::host_platform::DeleteHostFile(path, error);
        }
    }

    running_ = false;
    return ok;
}

}
