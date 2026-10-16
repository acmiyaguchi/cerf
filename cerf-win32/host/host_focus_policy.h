#pragma once

#include <cerfcore/core/service.h>

#define NOMINMAX
#include <windows.h>

class HostFocusPolicy : public cerfcore::Service {
public:
    using cerfcore::Service::Service;

    bool MayActivate() const;

    void Show(HWND hwnd) const;
    void Raise(HWND hwnd) const;
    void Focus(HWND hwnd) const;
};
