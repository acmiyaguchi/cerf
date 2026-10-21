#include <cerffrontend/host_menu_model.h>

#include <cerffrontend/frontend_dialogs.h>
#include <cerffrontend/frontend_window.h>
#include <cerffrontend/host_key_binding.h>
#include <cerffrontend/input_capture.h>
#include <cerffrontend/launcher_windows.h>
#include <cerffrontend/link_opener.h>
#include <cerffrontend/screenshots.h>
#include <cerffrontend/widget_menu_builder.h>

#include <cerfcore/boot/guest_cold_boot.h>
#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/host/emulation_pause.h>
#include <cerfcore/socs/guest_cpu_reset.h>
#include <cerfcore/state/hibernation.h>

#include <functional>
#include <string>
#include <utility>

namespace cerffrontend {

REGISTER_SERVICE(HostMenuModel);

namespace {

constexpr const char* kArticlesUrl = "https://cerf.cx/articles/";

MenuItem Item(std::string label, std::function<void()> on_click) {
    MenuItem item;
    item.label    = std::move(label);
    item.on_click = std::move(on_click);
    return item;
}

MenuItem Check(std::string label, bool checked, bool radio, std::function<void()> on_click) {
    MenuItem item = Item(std::move(label), std::move(on_click));
    item.checked  = checked;
    item.radio    = radio;
    return item;
}

}

const char* HostMenuModel::Title(int index) const {
    switch (index) {
        case 0: return "Actions";
        case 1: return "View";
        case 2: return "Help";
    }
    return "";
}

std::vector<MenuItem> HostMenuModel::Build(int index) {
    switch (index) {
        case 0: return Actions();
        case 1: return View();
        case 2: return Help();
    }
    return {};
}

void HostMenuModel::PickStateFile(bool save) {
    auto&             hibernation = emu_.Get<cerfcore::Hibernation>();
    const std::string path =
        emu_.Get<FrontendDialogs>().ChooseStateFile(save, hibernation.DefaultStatePath());
    if (path.empty()) return;
    if (save) hibernation.SaveAsync(path);
    else      hibernation.RestoreAsync(path, false);
}

void HostMenuModel::ConfirmHardReset() {
    if (emu_.Get<FrontendDialogs>().ConfirmWarning(
            "Hard reset - CE Runtime Foundation",
            "Hard reset clears all guest RAM.\nUnsaved guest data and the object store are "
            "lost.\n\nContinue?"))
        emu_.Get<cerfcore::GuestColdBoot>().RequestHardReset();
}

std::vector<MenuItem> HostMenuModel::Actions() {
    auto&       key   = emu_.Get<HostKeyBinding>();
    auto&       pause = emu_.Get<cerfcore::EmulationPause>();
    std::vector<MenuItem> items;
    items.push_back(Item("Save state...", [this] { PickStateFile(true); }));
    items.push_back(Item("Load state...", [this] { PickStateFile(false); }));
    items.push_back(MenuItem{});

    MenuItem toggle = Item((pause.IsUserPaused() ? "Resume\t" : "Pause\t") + key.LabelWith("P"),
                           [this] { emu_.Get<cerfcore::EmulationPause>().Toggle(); });
    toggle.enabled = pause.UserCanToggle();
    items.push_back(std::move(toggle));
    items.push_back(Item("Send Ctrl+Alt+Del\t" + key.LabelWith("Del"),
                         [this] { emu_.Get<InputCapture>().SendCtrlAltDel(); }));
    items.push_back(MenuItem{});
    items.push_back(Item("Soft reset", [this] { emu_.Get<cerfcore::GuestCpuReset>().WarmReset(); }));
    items.push_back(Item("Hard reset...", [this] { ConfirmHardReset(); }));
    items.push_back(MenuItem{});
    items.push_back(Item("Settings...", [this] { emu_.Get<LauncherWindows>().OpenSettings(); }));
    items.push_back(MenuItem{});
    for (MenuItem& widget : emu_.Get<WidgetMenuBuilder>().BuildAllWidgets())
        items.push_back(std::move(widget));
    return items;
}

ViewportMode HostMenuModel::MainViewport::Mode() const { return emu_.Get<FrontendWindow>().Mode(); }

int HostMenuModel::MainViewport::IntegerFactor() const {
    return emu_.Get<FrontendWindow>().IntegerFactor();
}

bool HostMenuModel::MainViewport::Antialias() const {
    return emu_.Get<FrontendWindow>().Antialias();
}

void HostMenuModel::MainViewport::SetViewportMode(ViewportMode m) {
    emu_.Get<FrontendWindow>().SetViewportMode(m);
}

void HostMenuModel::MainViewport::SetIntegerScale(int factor) {
    emu_.Get<FrontendWindow>().SetIntegerScale(factor);
}

void HostMenuModel::MainViewport::SetAntialias(bool on) {
    emu_.Get<FrontendWindow>().SetAntialias(on);
}

bool HostMenuModel::MainViewport::IsFullscreen() const {
    return emu_.Get<FrontendWindow>().IsFullscreen();
}

void HostMenuModel::MainViewport::ToggleFullscreen() {
    emu_.Get<FrontendWindow>().ToggleFullscreen();
}

void HostMenuModel::MainViewport::SaveScreenshot() { emu_.Get<Screenshots>().Save(); }

void HostMenuModel::MainViewport::CopyScreenshot() { emu_.Get<Screenshots>().Copy(); }

void HostMenuModel::MainViewport::AfterModeChange(bool) {
    emu_.Get<FrontendWindow>().RefitIfFollowingGuest();
}

std::vector<MenuItem> HostMenuModel::View() {
    auto&                     win = emu_.Get<FrontendWindow>();
    const FrontendWindow::Tab tab = win.CurrentTab();

    auto tab_item = [&](const char* label, FrontendWindow::Tab t) {
        return Check(label, tab == t, true,
                     [this, t] { emu_.Get<FrontendWindow>().SetTab(t, true); });
    };

    std::vector<MenuItem> items;
    items.push_back(tab_item("Boot Screen", FrontendWindow::Tab::Boot));
    items.push_back(tab_item("Hardware Screen", FrontendWindow::Tab::Hw));
    items.push_back(tab_item("Framebuffer", FrontendWindow::Tab::Framebuffer));
    items.push_back(MenuItem{});
    emu_.Get<ViewportMenu>().Append(items, main_viewport_);
    items.push_back(MenuItem{});
    items.push_back(Check("Match guest size", win.FollowGuest(), false,
                          [this] { emu_.Get<FrontendWindow>().MatchGuestSize(); }));
    return items;
}

std::vector<MenuItem> HostMenuModel::Help() {
    std::vector<MenuItem> items;
    items.push_back(Item("Articles (opens in browser)",
                         [this] { emu_.Get<LinkOpener>().Open(kArticlesUrl); }));
    items.push_back(MenuItem{});
    items.push_back(Item("About", [this] { emu_.Get<LauncherWindows>().OpenAbout(); }));
    return items;
}

}
