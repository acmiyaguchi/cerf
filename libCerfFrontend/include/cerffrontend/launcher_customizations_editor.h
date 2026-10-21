#pragma once

#include <cerfcore/host/customizations_editor.h>

#include <cstdint>
#include <string>

namespace cerffrontend {

class LauncherCustomizationsEditor : public cerfcore::CustomizationsEditor {
public:
    using cerfcore::CustomizationsEditor::CustomizationsEditor;

    bool ShouldRegister() override;

    void Open() override;
    bool Open(uint64_t owner, bool force_reboot);

private:
    void Apply(const std::string& reboot, uint32_t prev_w, uint32_t prev_h);
};

}
