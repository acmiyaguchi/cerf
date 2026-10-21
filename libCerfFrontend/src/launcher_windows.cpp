#include <cerffrontend/launcher_windows.h>

#include <cerffrontend/frontend_window.h>
#include <cerffrontend/launcher_transaction.h>

#include <cerfcore/boards/board_context.h>
#include <cerfcore/core/cerf_emulator.h>

namespace cerffrontend {

REGISTER_SERVICE(LauncherWindows);

bool LauncherWindows::OpenAbout() {
    auto& board = emu_.Get<cerfcore::BoardContext>();

    nlohmann::json query;
    if (!board.GetBoardId().empty()) {
        query["board_name"] = board.BoardName();
        query["soc_name"]   = board.GetSocId().empty() ? "" : board.SocName();
    }
    nlohmann::json response;
    return emu_.Get<LauncherTransaction>().Run(emu_.Get<FrontendWindow>().NativeHandle(), "about",
                                               query, response);
}

bool LauncherWindows::OpenSettings() {
    const nlohmann::json query = nlohmann::json::object();
    nlohmann::json       response;
    return emu_.Get<LauncherTransaction>().Run(emu_.Get<FrontendWindow>().NativeHandle(),
                                               "settings", query, response);
}

}
