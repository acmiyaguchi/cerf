#include "win32_menu_binder.h"

#include <cerfcore/platform/wide_text_win32.h>

void Win32MenuBinder::AppendItems(HMENU menu, const std::vector<cerffrontend::MenuItem>& items) {
    for (const auto& it : items) {
        if (it.label.empty()) {
            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
            continue;
        }
        const std::wstring label = cerfcore::Utf8ToWide(it.label.c_str());
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
        if (it.radio && it.checked) {
            MENUITEMINFOW mii = { sizeof(mii) };
            mii.fMask = MIIM_FTYPE;
            mii.fType = MFT_STRING | MFT_RADIOCHECK;
            SetMenuItemInfoW(menu, GetMenuItemCount(menu) - 1, TRUE, &mii);
        }
    }
}

HMENU Win32MenuBinder::BuildContextMenu(const std::vector<cerffrontend::MenuItem>& items) {
    id_callbacks_.clear();
    HMENU m = CreatePopupMenu();
    AppendItems(m, items);
    return m;
}

void Win32MenuBinder::Populate(HMENU menu, const std::vector<cerffrontend::MenuItem>& items) {
    id_callbacks_.clear();
    while (GetMenuItemCount(menu) > 0) DeleteMenu(menu, 0, MF_BYPOSITION);
    AppendItems(menu, items);
}

void Win32MenuBinder::Dispatch(int id) {
    const size_t idx = static_cast<size_t>(id - kIdBase);
    std::function<void()> cb;
    if (idx < id_callbacks_.size()) cb = id_callbacks_[idx];
    if (cb) cb();
}
