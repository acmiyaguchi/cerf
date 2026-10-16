#pragma once

#include <cerfcore/core/service.h>

#define NOMINMAX
#include <windows.h>

#include <map>
#include <string>
#include <utility>

/* Renders cerf.rc ICON resources for status-bar widgets, sized to fit the
   widget box, with a per-(resource,size) HICON cache. UI-thread only. */
class HostIconCache : public Service {
public:
    using Service::Service;
    ~HostIconCache() override;

    void DrawCentered(HDC dc, const RECT& box, const std::string& res_name);

private:
    HICON Resolve(const std::string& res_name, int px);

    std::map<std::pair<std::string, int>, HICON> cache_;
};
