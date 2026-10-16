#include <cerffrontend/frontend_config.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/cerf_paths.h>
#include <cerfcore/core/config_json.h>
#include <cerfcore/core/host_fs_path.h>
#include <cerfcore/core/log.h>

#include <nlohmann/json.hpp>

#include <fstream>

REGISTER_SERVICE(FrontendConfig);

namespace {

using nlohmann::json;

std::vector<uint8_t> ReadHostKey(const json& root, const std::string& path) {
    std::vector<uint8_t> vks;
    const char* k = "host_key";
    if (!root.contains(k)) return vks;
    const auto& v = root[k];
    if (v.is_null()) return vks;

    auto add = [&](const json& e) {
        if (!e.is_number_integer())
            CfgFatal(path, "'host_key' entries must be virtual-key code integers");
        const int n = e.get<int>();
        if (n < 1 || n > 255)
            CfgFatal(path, "'host_key' virtual-key code out of range (1..255)");
        if (n == 0x01 || n == 0x02 || n == 0x04 || n == 0x05 || n == 0x06)
            CfgFatal(path, "'host_key' mouse buttons never reach the keyboard "
                           "hook; name a keyboard virtual-key code");
        if (n == 0x10)
            CfgFatal(path, "'host_key' 16 (Shift) never reaches the keyboard "
                           "hook; name 160 (left Shift) or 161 (right Shift)");
        if (n == 0x11)
            CfgFatal(path, "'host_key' 17 (Ctrl) never reaches the keyboard "
                           "hook; name 162 (left Ctrl) or 163 (right Ctrl)");
        if (n == 0x12)
            CfgFatal(path, "'host_key' 18 (Alt) never reaches the keyboard "
                           "hook; name 164 (left Alt) or 165 (right Alt)");
        for (uint8_t have : vks)
            if (have == (uint8_t)n) return;
        vks.push_back((uint8_t)n);
    };

    if (v.is_number_integer()) {
        add(v);
    } else if (v.is_array()) {
        for (const auto& e : v) add(e);
    } else {
        CfgFatal(path, "'host_key' must be a virtual-key code integer or an "
                       "array of them");
    }
    if (vks.empty())
        CfgFatal(path, "'host_key' must name at least one virtual-key code");
    if (vks.size() > 8)
        CfgFatal(path, "'host_key' must name at most 8 virtual-key codes");
    return vks;
}

}

void FrontendConfig::OnReady() {
    path_ = GetCerfDir() + "cerf.json";
    const json j = CfgReadLoggedJsonFile(path_);
    if (j.is_null()) return;

    if (j.contains("device")) {
        if (!j["device"].is_string())
            CfgFatal(path_, "'device' must be a string");
        device_ = j["device"].get<std::string>();
    }
    if (j.contains("last_save_state_mode")) {
        if (!j["last_save_state_mode"].is_boolean())
            CfgFatal(path_, "'last_save_state_mode' must be a boolean");
        last_save_state_mode_ = j["last_save_state_mode"].get<bool>();
    }
    if (j.contains("discord_rich_presence")) {
        if (!j["discord_rich_presence"].is_boolean())
            CfgFatal(path_, "'discord_rich_presence' must be a boolean");
        discord_rich_presence_ = j["discord_rich_presence"].get<bool>();
    }
    host_key_vks_ = ReadHostKey(j, path_);
}

void FrontendConfig::SaveLastSaveStateMode(bool save_state) {
    json j = CfgReadJsonFile(path_);
    if (!j.is_object()) j = json::object();
    j["last_save_state_mode"] = save_state;

    last_save_state_mode_ = save_state;

    std::ofstream f(HostFsPath(path_), std::ios::trunc);
    if (!f.is_open()) {
        LOG(Cfg, "Could not write '%s' to persist last_save_state_mode\n",
            path_.c_str());
        return;
    }
    f << j.dump(2) << '\n';
}
