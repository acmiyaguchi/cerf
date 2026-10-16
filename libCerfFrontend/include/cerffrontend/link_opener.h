#pragma once

#include <cerfcore/core/service.h>

#include <string>

class LinkOpener : public Service {
public:
    using Service::Service;

    virtual void Open(const std::string& url) = 0;
};
