#include <cerffrontend/frontend_window.h>
#include <cerffrontend/image_exporter.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/cerf_paths.h>
#include <cerfcore/core/device_config.h>
#include <cerfcore/core/run_timeout.h>
#include <cerfcore/core/service.h>
#include <cerfcore/platform/host_file.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace cerffrontend {

namespace {

constexpr auto kInterval    = std::chrono::seconds(10);
constexpr auto kCaptureWait = std::chrono::seconds(3);

class LiveScreenshotReporter : public cerfcore::Service {
public:
    using cerfcore::Service::Service;
    ~LiveScreenshotReporter() override { Stop(); }

    void OnReady() override {
        const std::string name = emu_.Get<cerfcore::DeviceConfig>().device_name;
        if (name.empty()) return;
        path_   = cerfcore::GetDeviceDir(name) + "live_state.png";
        thread_ = std::thread([this] { Loop(); });
    }

    void OnShutdown() override { Stop(); }

private:
    void Stop() {
        stop_.store(true, std::memory_order_release);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            cv_.notify_all();
        }
        {
            std::lock_guard<std::mutex> lock(capture_mutex_);
            capture_cv_.notify_all();
        }
        if (thread_.joinable()) thread_.join();
        if (!path_.empty() && !cerfcore::RunTimeout::IsActive()) {
            uint32_t error = 0;
            cerfcore::host_platform::DeleteHostFile(path_, error);
        }
    }

    void Loop() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!stop_.load(std::memory_order_acquire)) {
            lock.unlock();
            Tick();
            lock.lock();
            if (stop_.load(std::memory_order_acquire)) break;
            cv_.wait_for(lock, kInterval);
        }
    }

    void Tick() {
        uint64_t serial = 0;
        {
            std::lock_guard<std::mutex> lock(capture_mutex_);
            serial    = ++capture_serial_;
            captured_ = false;
        }
        emu_.Get<FrontendWindow>().RunOnUiThread([this, serial] {
            std::vector<uint32_t> pixels;
            uint32_t              w = 0, h = 0;
            const bool captured = emu_.Get<FrontendWindow>().CaptureGuestSurface(pixels, w, h);
            std::lock_guard<std::mutex> lock(capture_mutex_);
            if (serial != capture_serial_) return;
            pixels_       = std::move(pixels);
            w_            = w;
            h_            = h;
            captured_     = captured;
            capture_done_ = serial;
            capture_cv_.notify_all();
        });
        std::unique_lock<std::mutex> lock(capture_mutex_);
        const bool done = capture_cv_.wait_for(lock, kCaptureWait, [&] {
            return capture_done_ == serial || stop_.load(std::memory_order_acquire);
        });
        if (!done || stop_.load(std::memory_order_acquire) || !captured_) return;
        const std::vector<uint32_t> pixels = std::move(pixels_);
        const uint32_t              w      = w_;
        const uint32_t              h      = h_;
        lock.unlock();
        emu_.Get<ImageExporter>().WritePng(pixels, w, h, path_);
    }

    std::string             path_;
    std::thread             thread_;
    std::mutex              mutex_;
    std::condition_variable cv_;
    std::atomic<bool>       stop_{false};
    std::mutex              capture_mutex_;
    std::condition_variable capture_cv_;
    uint64_t                capture_serial_ = 0;
    uint64_t                capture_done_   = 0;
    bool                    captured_       = false;
    std::vector<uint32_t>   pixels_;
    uint32_t                w_ = 0;
    uint32_t                h_ = 0;
};

}

REGISTER_SERVICE(LiveScreenshotReporter);

}
