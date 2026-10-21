#pragma once

#include <cerfcore/core/service.h>

#include <cstdint>
#include <string>
#include <vector>

namespace cerffrontend {

class LauncherProcess : public cerfcore::Service {
public:
    using cerfcore::Service::Service;

    virtual std::string ExecutablePath() const = 0;
    virtual bool        RunAndWait(uint64_t owner, const std::vector<std::string>& args,
                                   uint32_t& exit_code, std::string& error) = 0;
};

}
