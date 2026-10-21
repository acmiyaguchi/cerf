#pragma once

#include <cerfcore/core/service.h>

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>

namespace cerffrontend {

class LauncherTransaction : public cerfcore::Service {
public:
    using cerfcore::Service::Service;

    bool Run(uint64_t owner, const char* scaffolding, const nlohmann::json& query,
             nlohmann::json& response);

private:
    void Complain(const std::string& text);
    bool LocateLauncher(std::string& exe);
    bool WriteRequest(const std::string& path, const nlohmann::json& request);
    bool Spawn(uint64_t owner, const std::string& exe, const std::string& device,
               const std::string& file);
    bool ReadResponse(const std::string& path, const char* key, nlohmann::json& response);

    bool     running_ = false;
    uint32_t serial_  = 0;
};

}
