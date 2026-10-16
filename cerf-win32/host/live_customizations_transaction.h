#pragma once

#include <cerfcore/host/customizations_editor.h>

#define NOMINMAX
#include <windows.h>

#include <cstdint>
#include <string>

class LiveCustomizationsTransaction : public CustomizationsEditor {
public:
    using CustomizationsEditor::CustomizationsEditor;

    bool ShouldRegister() override;

    void Open() override;
    bool Open(HWND owner, bool force_reboot);

private:
    void Apply(std::string reboot, uint32_t prev_w, uint32_t prev_h);
};
