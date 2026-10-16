#pragma once

#include <cerfcore/core/service.h>
#include <cerfcore/host/widget_control.h>
#include "widget_menu_model.h"

#include <vector>

class WidgetMenuBuilder : public Service {
public:
    using Service::Service;

    std::vector<MenuItem> Build(const std::vector<WidgetControl>& controls);

private:
    void Append(std::vector<MenuItem>& out, const std::vector<WidgetControl>& controls);
    void AppendSlot(std::vector<MenuItem>& out, const SlotControl& slot);
    std::vector<MenuItem> Entries(const std::vector<SlotEntry>& entries);
};
