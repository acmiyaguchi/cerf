#include <cerffrontend/window_title.h>

#include <cerfcore/core/board_database.h>
#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/device_config.h>
#include <cerfcore/core/device_meta_label.h>
#include <cerffrontend/version.h>

#include <vector>

REGISTER_SERVICE(WindowTitle);

std::string WindowTitle::Compose() const {
    const DeviceConfig& cfg = emu_.Get<DeviceConfig>();
    const DeviceMeta& meta = cfg.meta;

    std::vector<std::string> parts;
    if (!meta.name.empty())
        parts.push_back(meta.name);
    else if (!meta.device_name.empty())
        parts.push_back(meta.device_name);
    else if (const DbBoard* board =
                 emu_.Get<BoardDatabase>().FindBoard(cfg.board_id))
        parts.push_back(board->name);
    const std::string os = OsDisplayLabel(meta);
    if (!os.empty())
        parts.push_back(os);
    parts.push_back("CERF " CERF_VERSION_DISPLAY_STR);

    std::string title;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i) title += " \xE2\x80\xA2 ";
        title += parts[i];
    }
    return title;
}
