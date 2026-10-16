#pragma once

#include <cerfcore/core/service.h>

#include <string>

class WindowTitle : public Service {
public:
    using Service::Service;

    std::string Compose() const;
};
