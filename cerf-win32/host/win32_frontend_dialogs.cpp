#define NOMINMAX

#include <cerffrontend/frontend_dialogs.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/platform/wide_text_win32.h>
#include "host_window.h"

#include <windows.h>
#include <commdlg.h>

namespace {

class Win32FrontendDialogs : public cerffrontend::FrontendDialogs {
public:
    using cerffrontend::FrontendDialogs::FrontendDialogs;

    std::string ChooseStateFile(bool save, const std::string& default_path) override {
        wchar_t path[MAX_PATH];
        lstrcpynW(path, cerfcore::Utf8ToWide(default_path.c_str()).c_str(), MAX_PATH);
        OPENFILENAMEW ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner   = emu_.Get<HostWindow>().Hwnd();
        ofn.lpstrFilter = L"State image (*.img)\0*.img\0All files (*.*)\0*.*\0";
        ofn.lpstrFile   = path;
        ofn.nMaxFile    = MAX_PATH;
        ofn.lpstrDefExt = L"img";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR |
                    (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
        const BOOL ok = save ? GetSaveFileNameW(&ofn) : GetOpenFileNameW(&ofn);
        return ok ? cerfcore::WideToUtf8(path) : std::string();
    }

    bool ConfirmWarning(const std::string& title, const std::string& text) override {
        return MessageBoxW(emu_.Get<HostWindow>().Hwnd(), cerfcore::Utf8ToWide(text.c_str()).c_str(),
                           cerfcore::Utf8ToWide(title.c_str()).c_str(),
                           MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDYES;
    }
};

}

REGISTER_SERVICE_AS(Win32FrontendDialogs, cerffrontend::FrontendDialogs);
