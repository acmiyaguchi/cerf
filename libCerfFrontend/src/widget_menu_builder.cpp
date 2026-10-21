#include <cerffrontend/widget_menu_builder.h>

#include <cerffrontend/link_opener.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/host/host_widget.h>
#include <cerfcore/host/host_widget_registry.h>

#include <type_traits>
#include <utility>

namespace cerffrontend {

REGISTER_SERVICE(WidgetMenuBuilder);

namespace {

void Separate(std::vector<MenuItem>& out) {
    if (!out.empty() && !out.back().label.empty()) out.push_back(MenuItem{});
}

void Trim(std::vector<MenuItem>& out) {
    while (!out.empty() && out.back().label.empty()) out.pop_back();
}

const char* InsertLabel(cerfcore::SlotKind kind) {
    switch (kind) {
        case cerfcore::SlotKind::kPcCard:
        case cerfcore::SlotKind::kSerialPort: return "Insert card";
        case cerfcore::SlotKind::kUsbDisk:
        case cerfcore::SlotKind::kSdCard:     return "Insert media";
    }
    return "Insert";
}

}

std::vector<MenuItem> WidgetMenuBuilder::Build(
    const std::vector<cerfcore::WidgetControl>& controls) {
    std::vector<MenuItem> out;
    Append(out, controls);
    Trim(out);
    return out;
}

std::vector<MenuItem> WidgetMenuBuilder::BuildAllWidgets() {
    std::vector<MenuItem> out;
    const auto            ordered = emu_.Get<cerfcore::HostWidgetRegistry>().Ordered();
    for (auto it = ordered.rbegin(); it != ordered.rend(); ++it) {
        cerfcore::HostWidget* w     = *it;
        std::vector<MenuItem> items = Build(w->Controls());
        if (items.empty()) {
            MenuItem name;
            name.label   = w->WidgetName();
            name.enabled = false;
            out.push_back(std::move(name));
        } else if (items.size() == 1 && items[0].submenu.empty()) {
            out.push_back(std::move(items[0]));
        } else {
            MenuItem group;
            group.label   = w->WidgetName();
            group.submenu = std::move(items);
            out.push_back(std::move(group));
        }
    }
    return out;
}

void WidgetMenuBuilder::Append(std::vector<MenuItem>& out,
                               const std::vector<cerfcore::WidgetControl>& controls) {
    bool after_section = false;
    for (const cerfcore::WidgetControl& c : controls) {
        if (after_section) {
            Separate(out);
            after_section = false;
        }
        std::visit([&](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, cerfcore::InfoControl>) {
                MenuItem item;
                item.label   = v.text;
                item.enabled = false;
                out.push_back(std::move(item));
            } else if constexpr (std::is_same_v<T, cerfcore::LinkControl>) {
                MenuItem item;
                item.label    = v.label;
                item.on_click = [this, url = v.url] { emu_.Get<LinkOpener>().Open(url); };
                out.push_back(std::move(item));
            } else if constexpr (std::is_same_v<T, cerfcore::ActionControl>) {
                MenuItem item;
                item.label    = v.label;
                item.enabled  = v.enabled;
                item.on_click = v.press;
                out.push_back(std::move(item));
            } else if constexpr (std::is_same_v<T, cerfcore::ToggleControl>) {
                MenuItem item;
                item.label    = v.label;
                item.enabled  = v.enabled;
                item.checked  = v.value;
                if (v.set) item.on_click = [set = v.set, next = !v.value] { set(next); };
                out.push_back(std::move(item));
            } else if constexpr (std::is_same_v<T, cerfcore::ChoiceControl>) {
                for (const cerfcore::ChoiceOption& o : v.options) {
                    MenuItem item;
                    item.label    = o.label;
                    item.checked  = o.id == v.selected;
                    if (v.select) item.on_click = [select = v.select, id = o.id] { select(id); };
                    out.push_back(std::move(item));
                }
            } else if constexpr (std::is_same_v<T, cerfcore::SlotControl>) {
                AppendSlot(out, v);
            } else if constexpr (std::is_same_v<T, cerfcore::GroupControl>) {
                if (v.label.empty()) {
                    Separate(out);
                    Append(out, v.children);
                    after_section = true;
                } else {
                    MenuItem item;
                    item.label = v.label;
                    Append(item.submenu, v.children);
                    Trim(item.submenu);
                    out.push_back(std::move(item));
                }
            }
        }, c.control);
    }
}

void WidgetMenuBuilder::AppendSlot(std::vector<MenuItem>& out, const cerfcore::SlotControl& slot) {
    if (!slot.inserted) {
        MenuItem insert;
        insert.label   = InsertLabel(slot.kind);
        insert.submenu = Entries(slot.available);
        out.push_back(std::move(insert));
        return;
    }

    MenuItem header;
    header.label   = slot.inserted->label;
    header.enabled = false;
    out.push_back(std::move(header));

    MenuItem eject;
    eject.label    = "Eject";
    eject.on_click = slot.eject;
    out.push_back(std::move(eject));

    MenuItem swap;
    swap.label   = "Eject and insert";
    swap.submenu = Entries(slot.available);
    out.push_back(std::move(swap));

    if (!slot.inserted_controls.empty()) {
        Separate(out);
        Append(out, slot.inserted_controls);
    }
}

std::vector<MenuItem> WidgetMenuBuilder::Entries(const std::vector<cerfcore::SlotEntry>& entries) {
    std::vector<MenuItem> out;
    for (const cerfcore::SlotEntry& e : entries) {
        MenuItem item;
        item.label   = e.label;
        item.enabled = e.enabled;
        if (e.variants.empty() && e.details.empty()) {
            item.on_click = e.insert;
            out.push_back(std::move(item));
            continue;
        }
        if (!e.variants.empty()) {
            item.submenu = Entries(e.variants);
        } else if (e.insert) {
            MenuItem insert;
            insert.label    = "Insert";
            insert.on_click = e.insert;
            item.submenu.push_back(std::move(insert));
        }
        if (!e.details.empty()) {
            Separate(item.submenu);
            Append(item.submenu, e.details);
            Trim(item.submenu);
        }
        out.push_back(std::move(item));
    }
    return out;
}

}
