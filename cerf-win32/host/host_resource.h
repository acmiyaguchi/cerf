#pragma once

#include <cerfcore/core/service.h>

#define NOMINMAX
#include <windows.h>

#include <cstdint>
#include <span>

class HostResource : public cerfcore::Service {
public:
    using cerfcore::Service::Service;

    std::span<const uint8_t> Bytes(const wchar_t* name) const;
};
