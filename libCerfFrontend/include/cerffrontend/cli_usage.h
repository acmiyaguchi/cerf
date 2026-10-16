#pragma once

#include <cerfcore/core/service.h>

class CliUsage : public Service {
public:
    using Service::Service;

    void Print(const char* prog);
};
