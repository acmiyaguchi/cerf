#pragma once

#include <cerfcore/core/service.h>

#define NOMINMAX
#include <windows.h>

#include <vector>

class HostFonts : public cerfcore::Service {
public:
    using cerfcore::Service::Service;

    void OnReady() override;
    void OnShutdown() override;

    const wchar_t* UiFace() const;
    const wchar_t* MonoFace() const;

private:
    bool AddResourceFace(const wchar_t* resource_name);

    std::vector<HANDLE> faces_;
    bool ui_ok_   = false;
    bool mono_ok_ = false;
};
