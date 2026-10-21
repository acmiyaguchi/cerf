#include <cerffrontend/frontend_window.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/cerf_paths.h>
#include <cerfcore/core/device_config.h>
#include <cerfcore/core/service.h>
#include <cerfcore/core/utf16.h>
#include <cerfcore/platform/host_file.h>
#include <cerfcore/platform/host_process.h>

#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>

namespace cerffrontend {

namespace {

constexpr auto kHeartbeatInterval = std::chrono::seconds(2);

int64_t NowUnix() {
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

class LauncherStatusReporter : public cerfcore::Service {
public:
    using cerfcore::Service::Service;
    ~LauncherStatusReporter() override { Stop(); }

    void OnReady() override {
        const std::string name = emu_.Get<cerfcore::DeviceConfig>().device_name;
        if (name.empty()) return;
        status_path_  = cerfcore::GetDeviceDir(name) + "cerf-status.json";
        started_unix_ = NowUnix();
        thread_       = std::thread([this] { HeartbeatLoop(); });
    }

    void OnShutdown() override { Stop(); }

private:
    void Stop() {
        stop_.store(true, std::memory_order_release);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            cv_.notify_all();
        }
        if (thread_.joinable()) thread_.join();
        if (!status_path_.empty()) {
            uint32_t error = 0;
            cerfcore::host_platform::DeleteHostFile(status_path_, error);
        }
    }

    void HeartbeatLoop() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!stop_.load(std::memory_order_acquire)) {
            lock.unlock();
            WriteStatus();
            lock.lock();
            if (stop_.load(std::memory_order_acquire)) break;
            cv_.wait_for(lock, kHeartbeatInterval);
        }
    }

    void WriteStatus() {
        nlohmann::json j;
        j["pid"]            = cerfcore::host_platform::CurrentProcessId();
        j["hwnd"]           = emu_.Get<FrontendWindow>().NativeHandle();
        j["started_unix"]   = started_unix_;
        j["heartbeat_unix"] = NowUnix();
        j["state"]          = "running";
        const std::string text = j.dump();

        const std::string tmp = status_path_ + ".tmp";
        {
            std::ofstream f(std::filesystem::path(cerfcore::Utf8ToUtf16(tmp)),
                            std::ios::binary | std::ios::trunc);
            if (!f) return;
            f.write(text.data(), static_cast<std::streamsize>(text.size()));
        }
        cerfcore::host_platform::ReplaceHostFile(tmp, status_path_, true);
    }

    std::string             status_path_;
    int64_t                 started_unix_ = 0;
    std::thread             thread_;
    std::mutex              mutex_;
    std::condition_variable cv_;
    std::atomic<bool>       stop_{false};
};

}

REGISTER_SERVICE(LauncherStatusReporter);

}
