#pragma once

#include <cerffrontend/link_opener.h>

#define NOMINMAX
#include <windows.h>

class HostLinkOpener : public cerffrontend::LinkOpener {
public:
    using cerffrontend::LinkOpener::LinkOpener;

    void Open(const std::string& url) override;
    void Open(HWND owner, const wchar_t* url);

    bool OpenNotified(HWND owner, LPARAM syslink_notify);
};
