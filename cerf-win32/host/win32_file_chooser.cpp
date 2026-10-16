#define NOMINMAX
#include <windows.h>
#include <commdlg.h>

#include <cerfcore/host/file_chooser.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/platform/wide_text_win32.h>
#include "host_window.h"

namespace {

constexpr DWORD kOpenFlags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY |
                             OFN_NOCHANGEDIR;

std::wstring FilterString(const std::vector<FileChooserFilter>& filters) {
    std::wstring out;
    for (const auto& f : filters) {
        out += Utf8ToWide(f.description.c_str());
        out += L'\0';
        out += Utf8ToWide(f.patterns.c_str());
        out += L'\0';
    }
    out += L'\0';
    return out;
}

class Win32FileChooser : public FileChooser {
public:
    using FileChooser::FileChooser;

    std::string ChooseFile(const std::string& title,
                           const std::vector<FileChooserFilter>& filters) override {
        wchar_t file[MAX_PATH] = {};
        const std::wstring filter = FilterString(filters);
        const std::wstring wide_title = Utf8ToWide(title.c_str());
        OPENFILENAMEW ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner   = emu_.Get<HostWindow>().Hwnd();
        ofn.lpstrFilter = filter.c_str();
        ofn.lpstrFile   = file;
        ofn.nMaxFile    = MAX_PATH;
        ofn.lpstrTitle  = wide_title.c_str();
        ofn.Flags       = kOpenFlags;
        return GetOpenFileNameW(&ofn) ? WideToUtf8(file) : std::string();
    }

    std::vector<std::string> ChooseFiles(const std::string& title,
                                         const std::vector<FileChooserFilter>& filters) override {
        std::vector<std::string> out;
        std::vector<wchar_t> buf(64u * 1024u, 0);
        const std::wstring filter = FilterString(filters);
        const std::wstring wide_title = Utf8ToWide(title.c_str());
        OPENFILENAMEW ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner   = emu_.Get<HostWindow>().Hwnd();
        ofn.lpstrFilter = filter.c_str();
        ofn.lpstrFile   = buf.data();
        ofn.nMaxFile    = static_cast<DWORD>(buf.size());
        ofn.lpstrTitle  = wide_title.c_str();
        ofn.Flags       = kOpenFlags | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
        if (!GetOpenFileNameW(&ofn)) return out;

        const wchar_t* p = buf.data();
        std::wstring dir(p);
        p += dir.size() + 1;
        if (*p == L'\0') {
            out.push_back(WideToUtf8(dir));
            return out;
        }
        while (*p) {
            std::wstring f(p);
            p += f.size() + 1;
            out.push_back(WideToUtf8(dir + L"\\" + f));
        }
        return out;
    }
};

}

REGISTER_SERVICE_AS(Win32FileChooser, FileChooser);
