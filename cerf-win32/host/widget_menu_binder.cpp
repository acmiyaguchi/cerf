#include "widget_menu_binder.h"

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/platform/wide_text_win32.h>
#include <cerfcore/host/host_widget_registry.h>
#include <cerffrontend/widget_menu_builder.h>

REGISTER_SERVICE(WidgetMenuBinder);

void WidgetMenuBinder::AppendItems(HMENU menu, const std::vector<MenuItem>& items) {
    for (const auto& it : items) {
        if (it.label.empty()) {
            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
            continue;
        }
        const std::wstring label = Utf8ToWide(it.label.c_str());
        if (!it.submenu.empty()) {
            HMENU sub = CreatePopupMenu();
            AppendItems(sub, it.submenu);
            AppendMenuW(menu, MF_POPUP | (it.enabled ? 0u : MF_GRAYED),
                        reinterpret_cast<UINT_PTR>(sub), label.c_str());
            continue;
        }
        UINT flags = MF_STRING;
        if (!it.enabled || !it.on_click) flags |= MF_GRAYED;
        if (it.checked)                  flags |= MF_CHECKED;
        UINT id = 0;
        if (it.on_click && it.enabled) {
            id = static_cast<UINT>(kIdBase + id_callbacks_.size());
            id_callbacks_.push_back(it.on_click);
        }
        AppendMenuW(menu, flags, id, label.c_str());
    }
}

HMENU WidgetMenuBinder::BuildContextMenu(const std::vector<MenuItem>& items) {
    id_callbacks_.clear();
    HMENU m = CreatePopupMenu();
    AppendItems(m, items);
    return m;
}

void WidgetMenuBinder::AppendAllToMenu(HMENU dest) {
    id_callbacks_.clear();
    auto ordered = emu_.Get<HostWidgetRegistry>().Ordered();
    for (auto it = ordered.rbegin(); it != ordered.rend(); ++it) {
        HostWidget* w = *it;
        auto items = emu_.Get<WidgetMenuBuilder>().Build(w->Controls());
        const std::wstring name = Utf8ToWide(w->WidgetName().c_str());
        if (items.empty()) {
            AppendMenuW(dest, MF_STRING | MF_GRAYED, 0, name.c_str());
        } else if (items.size() == 1 && items[0].submenu.empty()) {
            AppendItems(dest, items);
        } else {
            HMENU sub = CreatePopupMenu();
            AppendItems(sub, items);
            AppendMenuW(dest, MF_POPUP, reinterpret_cast<UINT_PTR>(sub), name.c_str());
        }
    }
}

void WidgetMenuBinder::Dispatch(int id) {
    const size_t idx = static_cast<size_t>(id - kIdBase);
    std::function<void()> cb;
    if (idx < id_callbacks_.size()) cb = id_callbacks_[idx];
    if (cb) cb();
}
