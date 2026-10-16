#pragma once

#include <cerfcore/core/service.h>
#include <cerffrontend/widget_menu_model.h>

#define NOMINMAX
#include <windows.h>

#include <functional>
#include <vector>

class WidgetMenuBinder : public cerfcore::Service {
public:
    using cerfcore::Service::Service;

    static constexpr int kIdBase = 0xC000;
    static constexpr int kIdEnd  = 0xCFFF;
    bool OwnsCommand(int id) const { return id >= kIdBase && id <= kIdEnd; }

    HMENU BuildContextMenu(const std::vector<cerffrontend::MenuItem>& items);
    void  AppendAllToMenu(HMENU dest);
    void  Dispatch(int id);

private:
    void AppendItems(HMENU menu, const std::vector<cerffrontend::MenuItem>& items);

    std::vector<std::function<void()>> id_callbacks_;
};
