#include <cerffrontend/status_bar.h>

#include <cerffrontend/widget_icon_painter.h>
#include <cerffrontend/widget_menu_builder.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/host/host_widget.h>
#include <cerfcore/host/host_widget_registry.h>

#include <algorithm>

namespace cerffrontend {

REGISTER_SERVICE(StatusBar);

namespace {

constexpr Rgb     kBg{32, 32, 32};
constexpr Rgb     kSep{60, 60, 60};
constexpr Rgb     kBgLight{240, 240, 240};
constexpr Rgb     kSepLight{200, 200, 200};
constexpr Rgb     kRx{229, 80, 80};
constexpr Rgb     kTx{78, 201, 90};
constexpr Rgb     kDotRim{20, 20, 20};
constexpr int     kGlowTicks     = 3;
constexpr int     kDotR          = 3;
constexpr uint8_t kDisabledAlpha = 150;

}

bool StatusBar::SampleActivity(cerfcore::HostWidget* w) {
    Glow& g = glow_[w];
    if (w->TakeRx())   g.rx = kGlowTicks;
    else if (g.rx > 0) --g.rx;
    if (w->TakeTx())   g.tx = kGlowTicks;
    else if (g.tx > 0) --g.tx;

    const bool now     = g.rx > 0 || g.tx > 0;
    const bool repaint = now || g.was_active;
    g.was_active       = now;
    return repaint;
}

bool StatusBar::Relayout(int width, int height) {
    const std::vector<cerfcore::HostWidget*> ordered =
        emu_.Get<cerfcore::HostWidgetRegistry>().Ordered();
    const int total_slots = width > 0 ? width / kIconBoxW : 0;

    int term_count = 0;
    for (auto* w : ordered)
        if (w->Group() >= cerfcore::WidgetGroup::GuestAdditions) ++term_count;
    const int dev_count = static_cast<int>(ordered.size()) - term_count;

    const int term_show = std::min(term_count, total_slots);
    const int dev_slots = std::max(total_slots - term_show, 0);
    const int dev_show  = std::min(dev_count, dev_slots);

    std::vector<cerfcore::HostWidget*> visible;
    visible.reserve(static_cast<size_t>(dev_show + term_show));
    for (int i = 0; i < dev_show; ++i) visible.push_back(ordered[static_cast<size_t>(i)]);
    for (int j = term_count - term_show; j < term_count; ++j)
        visible.push_back(ordered[static_cast<size_t>(dev_count + j)]);

    const int          block_left = width - static_cast<int>(visible.size()) * kIconBoxW;
    std::vector<Entry> next;
    next.reserve(visible.size());
    for (size_t k = 0; k < visible.size(); ++k) {
        const int left = block_left + static_cast<int>(k) * kIconBoxW;
        next.push_back({visible[k], Rect{left, 0, left + kIconBoxW, height}});
    }

    bool changed = next.size() != layout_.size();
    for (size_t i = 0; !changed && i < next.size(); ++i)
        changed = next[i].widget != layout_[i].widget || !(next[i].box == layout_[i].box);
    layout_.swap(next);
    return changed;
}

StatusBar::TickResult StatusBar::Tick(int width, int height) {
    TickResult r;
    for (cerfcore::HostWidget* w : emu_.Get<cerfcore::HostWidgetRegistry>().Ordered())
        r.repaint |= SampleActivity(w);
    r.layout_changed = Relayout(width, height);
    r.repaint |= r.layout_changed;
    for (size_t i = 0; i < layout_.size(); ++i) {
        if (!layout_[i].widget->PollDirty()) continue;
        r.repaint = true;
        r.tooltips_changed.push_back(i);
    }
    return r;
}

void StatusBar::DrawComposited(Painter& p, cerfcore::HostWidget* w, const Rect& box, Rgb bar_bg) {
    emu_.Get<WidgetIconPainter>().Paint(p, box, w->Icon());
    if (!w->IsEnabled()) {
        p.BlendRect(box, bar_bg, kDisabledAlpha);
        return;
    }
    auto dot = [&](Rgb color, int cx, int cy) {
        p.Ellipse({cx - kDotR, cy - kDotR, cx + kDotR + 1, cy + kDotR + 1}, kDotRim, color);
    };
    const Glow& g = glow_[w];
    if (g.rx > 0) dot(kRx, box.left + kDotR + 1, box.bottom - kDotR - 2);
    if (g.tx > 0) dot(kTx, box.right - kDotR - 2, box.bottom - kDotR - 2);
}

void StatusBar::Paint(Painter& p, const Rect& client, bool dark) {
    const Rgb bar_bg = dark ? kBg : kBgLight;
    p.FillRect(client, bar_bg);
    const Point separator[] = {{client.left, client.top}, {client.right, client.top}};
    p.Polyline(separator, 2, dark ? kSep : kSepLight, 1);
    for (const Entry& e : layout_) DrawComposited(p, e.widget, e.box, bar_bg);
}

std::string StatusBar::TooltipAt(size_t index) const {
    return index < layout_.size() ? layout_[index].widget->Tooltip() : std::string();
}

cerfcore::HostWidget* StatusBar::WidgetAt(int x) const {
    for (const Entry& e : layout_)
        if (x >= e.box.left && x < e.box.right) return e.widget;
    return nullptr;
}

bool StatusBar::WidgetBox(const cerfcore::HostWidget* w, Rect& out) const {
    for (const Entry& e : layout_) {
        if (e.widget != w) continue;
        out = e.box;
        return true;
    }
    return false;
}

bool StatusBar::CaptureWidgetBox(Rect& out) const {
    for (const Entry& e : layout_)
        if (e.widget->Group() == cerfcore::WidgetGroup::InputCapture)
            return WidgetBox(e.widget, out);
    return false;
}

bool StatusBar::PrimaryClick(cerfcore::HostWidget* w) {
    return w->TryPrimaryAction();
}

bool StatusBar::ContextMenu(cerfcore::HostWidget* w, std::vector<MenuItem>& out) {
    out = emu_.Get<WidgetMenuBuilder>().Build(w->Controls());
    if (w->ControlsAreOneAction() && out.size() == 1 && out[0].enabled && out[0].on_click &&
        out[0].submenu.empty()) {
        out[0].on_click();
        return false;
    }
    return true;
}

}
