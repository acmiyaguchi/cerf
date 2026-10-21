#include <cerfcore/host/host_displays.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/log.h>

#include <QGuiApplication>
#include <QObject>
#include <QScreen>

#include <atomic>
#include <cmath>

namespace {

class QtHostDisplays : public cerfcore::HostDisplays {
public:
    using cerfcore::HostDisplays::HostDisplays;

    void OnReady() override {
        for (QScreen* screen : QGuiApplication::screens()) Watch(screen);
        QObject::connect(qGuiApp, &QGuiApplication::screenAdded, &context_, [this](QScreen* screen) {
            Watch(screen);
            Recompute(nullptr);
        });
        QObject::connect(qGuiApp, &QGuiApplication::screenRemoved, &context_,
                         [this](QScreen* screen) { Recompute(screen); });
        Recompute(nullptr);
    }

    void LargestDisplaySize(uint32_t& width, uint32_t& height) override {
        const uint64_t size = size_.load(std::memory_order_acquire);
        width  = static_cast<uint32_t>(size >> 32);
        height = static_cast<uint32_t>(size);
    }

    int HighestRefreshRate() override { return refresh_.load(std::memory_order_acquire); }

private:
    void Watch(QScreen* screen) {
        QObject::connect(screen, &QScreen::geometryChanged, &context_, [this] { Recompute(nullptr); });
        QObject::connect(screen, &QScreen::refreshRateChanged, &context_, [this] { Recompute(nullptr); });
    }

    void Recompute(const QScreen* leaving) {
        uint32_t width   = 0;
        uint32_t height  = 0;
        int      refresh = 0;
        for (const QScreen* screen : QGuiApplication::screens()) {
            if (screen == leaving) continue;
            const qreal    ratio = screen->devicePixelRatio();
            const uint32_t w = static_cast<uint32_t>(std::lround(screen->size().width() * ratio));
            const uint32_t h = static_cast<uint32_t>(std::lround(screen->size().height() * ratio));
            if (static_cast<uint64_t>(w) * h > static_cast<uint64_t>(width) * height) {
                width  = w;
                height = h;
            }
            const int hz = static_cast<int>(std::lround(screen->refreshRate()));
            if (hz > refresh) refresh = hz;
        }
        size_.store((static_cast<uint64_t>(width) << 32) | height, std::memory_order_release);
        refresh_.store(refresh, std::memory_order_release);
        LOG(Lcd, "QtHostDisplays: largest %ux%u, highest %d Hz\n", width, height, refresh);
    }

    QObject               context_;
    std::atomic<uint64_t> size_{0};
    std::atomic<int>      refresh_{0};
};

}

REGISTER_SERVICE_AS(QtHostDisplays, cerfcore::HostDisplays);
