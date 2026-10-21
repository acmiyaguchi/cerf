#define NOMINMAX

#include "host_menu.h"

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/platform/wide_text_win32.h>
#include <cerffrontend/host_menu_model.h>

REGISTER_SERVICE(HostMenu);

HMENU HostMenu::Build() {
    HMENU bar   = CreateMenu();
    auto& model = emu_.Get<cerffrontend::HostMenuModel>();
    for (int i = 0; i < cerffrontend::HostMenuModel::kMenuCount; ++i)
        AppendMenuW(bar, MF_POPUP, (UINT_PTR)CreatePopupMenu(),
                    cerfcore::Utf8ToWide(model.Title(i)).c_str());
    bar_ = bar;
    return bar;
}

void HostMenu::OnInitMenuPopup(HMENU popup) {
    if (!bar_) return;
    for (int i = 0; i < cerffrontend::HostMenuModel::kMenuCount; ++i) {
        if (popup != GetSubMenu(bar_, i)) continue;
        binder_.Populate(popup, emu_.Get<cerffrontend::HostMenuModel>().Build(i));
        return;
    }
}

bool HostMenu::Dispatch(int id) {
    if (!binder_.OwnsCommand(id)) return false;
    binder_.Dispatch(id);
    return true;
}
