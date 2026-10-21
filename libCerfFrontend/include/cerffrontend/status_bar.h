#pragma once

#include <cerffrontend/painter.h>
#include <cerffrontend/widget_menu_model.h>

#include <cerfcore/core/service.h>

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace cerfcore { class HostWidget; }

namespace cerffrontend {

class StatusBar : public cerfcore::Service {
public:
    using cerfcore::Service::Service;

    static constexpr int      kHeight   = 24;
    static constexpr int      kIconBoxW = 26;
    static constexpr unsigned kTickMs   = 100;

    struct Entry {
        cerfcore::HostWidget* widget = nullptr;
        Rect                  box;
    };

    struct TickResult {
        bool                repaint        = false;
        bool                layout_changed = false;
        std::vector<size_t> tooltips_changed;
    };

    bool       Relayout(int width, int height);
    TickResult Tick(int width, int height);
    void       Paint(Painter& p, const Rect& client, bool dark);

    const std::vector<Entry>& Entries() const { return layout_; }
    std::string               TooltipAt(size_t index) const;
    cerfcore::HostWidget*     WidgetAt(int x) const;
    bool                      WidgetBox(const cerfcore::HostWidget* w, Rect& out) const;
    bool                      CaptureWidgetBox(Rect& out) const;

    bool PrimaryClick(cerfcore::HostWidget* w);
    bool ContextMenu(cerfcore::HostWidget* w, std::vector<MenuItem>& out);

private:
    struct Glow {
        int  rx         = 0;
        int  tx         = 0;
        bool was_active = false;
    };

    bool SampleActivity(cerfcore::HostWidget* w);
    void DrawComposited(Painter& p, cerfcore::HostWidget* w, const Rect& box, Rgb bar_bg);

    std::vector<Entry>                                 layout_;
    std::unordered_map<const cerfcore::HostWidget*, Glow> glow_;
};

}
