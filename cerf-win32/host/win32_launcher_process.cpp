#define NOMINMAX

#include <cerffrontend/launcher_process.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/cerf_paths.h>
#include <cerfcore/platform/wide_text_win32.h>

#include <windows.h>

#include <string>

namespace {

void AppendQuoted(std::wstring& cmd, const std::wstring& arg) {
    if (!cmd.empty()) cmd += L' ';
    cmd += L'"';
    size_t backslashes = 0;
    for (wchar_t c : arg) {
        if (c == L'\\') {
            ++backslashes;
            continue;
        }
        if (c == L'"') {
            cmd.append(backslashes * 2 + 1, L'\\');
        } else {
            cmd.append(backslashes, L'\\');
        }
        backslashes = 0;
        cmd += c;
    }
    cmd.append(backslashes * 2, L'\\');
    cmd += L'"';
}

void PumpUntilExit(HANDLE process) {
    for (;;) {
        const DWORD r = MsgWaitForMultipleObjects(1, &process, FALSE, INFINITE, QS_ALLINPUT);
        if (r != WAIT_OBJECT_0 + 1) return;
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                PostQuitMessage((int)msg.wParam);
                return;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
}

class Win32LauncherProcess : public cerffrontend::LauncherProcess {
public:
    using cerffrontend::LauncherProcess::LauncherProcess;

    std::string ExecutablePath() const override {
        return cerfcore::GetCerfDir() + "launcher\\launcher.exe";
    }

    bool RunAndWait(uint64_t owner, const std::vector<std::string>& args, uint32_t& exit_code,
                    std::string& error) override {
        const HWND   owner_hwnd = reinterpret_cast<HWND>(static_cast<uintptr_t>(owner));
        std::wstring cmd;
        AppendQuoted(cmd, cerfcore::Utf8ToWide(ExecutablePath().c_str()));
        for (const std::string& arg : args) AppendQuoted(cmd, cerfcore::Utf8ToWide(arg.c_str()));
        const std::wstring cwd = cerfcore::Utf8ToWide(cerfcore::GetCerfDir().c_str());

        STARTUPINFOW si = {};
        si.cb           = sizeof(si);
        PROCESS_INFORMATION pi = {};
        if (!CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, FALSE, 0, nullptr, cwd.c_str(),
                            &si, &pi)) {
            error = "launcher.exe error " + std::to_string(GetLastError());
            return false;
        }
        CloseHandle(pi.hThread);

        AllowSetForegroundWindow(pi.dwProcessId);
        PumpUntilExit(pi.hProcess);
        if (GetForegroundWindow() != owner_hwnd) SetForegroundWindow(owner_hwnd);

        DWORD code = 0;
        GetExitCodeProcess(pi.hProcess, &code);
        CloseHandle(pi.hProcess);
        exit_code = static_cast<uint32_t>(code);
        return true;
    }
};

}

REGISTER_SERVICE_AS(Win32LauncherProcess, cerffrontend::LauncherProcess);
