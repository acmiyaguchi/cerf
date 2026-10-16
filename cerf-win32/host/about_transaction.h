#pragma once

#include <cerfcore/core/service.h>

#define NOMINMAX
#include <windows.h>

class AboutTransaction : public Service {
public:
    using Service::Service;

    bool Open(HWND owner);
};
