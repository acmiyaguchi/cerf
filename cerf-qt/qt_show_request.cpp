#include "qt_main_window.h"

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/log.h>
#include <cerfcore/core/service.h>
#include <cerfcore/platform/host_process.h>

#include <QSocketNotifier>

#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#if !defined(__linux__)
#error "cerf-qt takes show requests on a Linux abstract-namespace socket; this host has none"
#endif

namespace {

class QtShowRequest : public cerfcore::Service {
public:
    using cerfcore::Service::Service;
    ~QtShowRequest() override { Close(); }

    void OnReady() override {
        fd_ = socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        if (fd_ < 0) {
            LOG(Caution, "QtShowRequest: socket failed (errno %d), the launcher cannot raise this window\n",
                errno);
            return;
        }
        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        const int len   = std::snprintf(addr.sun_path + 1, sizeof(addr.sun_path) - 1, "cerf-show/%u",
                                        cerfcore::host_platform::CurrentProcessId());
        const socklen_t size = static_cast<socklen_t>(offsetof(sockaddr_un, sun_path) + 1u +
                                                      static_cast<size_t>(len));
        if (bind(fd_, reinterpret_cast<const sockaddr*>(&addr), size) != 0) {
            LOG(Caution, "QtShowRequest: bind failed (errno %d), the launcher cannot raise this window\n",
                errno);
            Close();
            return;
        }
        notifier_ = new QSocketNotifier(fd_, QSocketNotifier::Read);
        QObject::connect(notifier_, &QSocketNotifier::activated, notifier_, [this] { Drain(); });
    }

    void OnShutdown() override { Close(); }

private:
    void Drain() {
        char buf[64];
        bool any = false;
        while (recv(fd_, buf, sizeof(buf), 0) >= 0) any = true;
        if (!any) return;
        LOG(Lcd, "QtShowRequest: show request, raising the window\n");
        emu_.Get<QtMainWindow>().Raise();
    }

    void Close() {
        delete notifier_;
        notifier_ = nullptr;
        if (fd_ >= 0) close(fd_);
        fd_ = -1;
    }

    int              fd_       = -1;
    QSocketNotifier* notifier_ = nullptr;
};

}

REGISTER_SERVICE(QtShowRequest);
