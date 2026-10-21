#pragma once

#include <cerfcore/core/service.h>
#include "win32_menu_binder.h"

#define NOMINMAX
#include <windows.h>

class HostMenu : public cerfcore::Service {
public:
    using cerfcore::Service::Service;

    HMENU Build();
    void  OnInitMenuPopup(HMENU popup);
    bool  Dispatch(int id);

private:
    HMENU           bar_ = nullptr;
    Win32MenuBinder binder_;
};
