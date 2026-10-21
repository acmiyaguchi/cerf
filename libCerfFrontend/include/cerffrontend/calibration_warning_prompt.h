#pragma once

#include <cerfcore/core/service.h>

namespace cerffrontend {

enum class CalibWarningChoice { kDismissed, kSwitchToStock };

class CalibrationWarningPrompt : public cerfcore::Service {
public:
    using cerfcore::Service::Service;

    virtual CalibWarningChoice Show()    = 0;
    virtual void               Dismiss() = 0;
};

}
