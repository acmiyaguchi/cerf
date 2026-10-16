#pragma once

#include <cerfcore/core/service.h>

#define NOMINMAX
#include <windows.h>

class AboutTransaction : public cerfcore::Service {
public:
    using cerfcore::Service::Service;

    bool Open(HWND owner);
};
