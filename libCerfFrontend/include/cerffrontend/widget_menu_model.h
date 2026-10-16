#pragma once

#include <functional>
#include <string>
#include <vector>

struct MenuItem {
    std::string           label;
    bool                  enabled = true;
    bool                  checked = false;
    std::function<void()> on_click;
    std::vector<MenuItem> submenu;
};
