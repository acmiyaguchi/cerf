#define NOMINMAX
#include "host_link_opener.h"

#include <commctrl.h>
#include <shellapi.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/platform/wide_text_win32.h>

REGISTER_SERVICE_AS(HostLinkOpener, cerffrontend::LinkOpener);

void HostLinkOpener::Open(const std::string& url) {
    Open(nullptr, cerfcore::Utf8ToWide(url.c_str()).c_str());
}

void HostLinkOpener::Open(HWND owner, const wchar_t* url) {
    ShellExecuteW(owner, L"open", url, nullptr, nullptr, SW_SHOWNORMAL);
}

bool HostLinkOpener::OpenNotified(HWND owner, LPARAM syslink_notify) {
    auto* link = reinterpret_cast<NMLINK*>(syslink_notify);
    if (!link->item.szUrl[0]) return false;
    Open(owner, link->item.szUrl);
    return true;
}
