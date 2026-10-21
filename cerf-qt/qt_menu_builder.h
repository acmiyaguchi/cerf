#pragma once

#include <cerffrontend/widget_menu_model.h>

#include <cerfcore/core/service.h>

#include <functional>
#include <utility>
#include <vector>

class QAction;
class QMenu;
class QPoint;

class QtMenuBuilder : public cerfcore::Service {
public:
    using cerfcore::Service::Service;

    void Populate(QMenu* menu, const std::vector<cerffrontend::MenuItem>& items);
    void RunContextMenu(const std::vector<cerffrontend::MenuItem>& items, const QPoint& global);

private:
    using Callbacks = std::vector<std::pair<QAction*, std::function<void()>>>;

    void Append(QMenu* menu, const std::vector<cerffrontend::MenuItem>& items, Callbacks& out);
};
