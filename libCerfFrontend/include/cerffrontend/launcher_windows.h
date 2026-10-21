#pragma once

#include <cerfcore/core/service.h>

namespace cerffrontend {

class LauncherWindows : public cerfcore::Service {
public:
    using cerfcore::Service::Service;

    bool OpenAbout();
    bool OpenSettings();
};

}
