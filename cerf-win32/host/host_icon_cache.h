#pragma once

#include <cerfcore/core/service.h>

#define NOMINMAX
#include <windows.h>

#include <map>
#include <string>
#include <utility>

/* Renders cerf.rc ICON resources for status-bar widgets, sized to fit the
   widget box, with a per-(resource,size) HICON cache. UI-thread only. */
class HostIconCache : public cerfcore::Service {
public:
    using cerfcore::Service::Service;
    ~HostIconCache() override;

    void DrawCentered(HDC dc, const RECT& box, const std::string& res_name);
    void DrawSized(HDC dc, const RECT& box, const std::string& res_name, int px);

private:
    HICON Resolve(const std::string& res_name, int px);

    std::map<std::pair<std::string, int>, HICON> cache_;
};
