#define NOMINMAX
#include <windows.h>

#include <cerfcore/host/number_prompt.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/platform/wide_text_win32.h>
#include "host_dark_mode.h"
#include "host_window.h"

#include <cstdint>
#include <vector>

namespace {

constexpr int kIdValueEdit = 1001;

struct PromptCtx {
    UINT          minimum;
    HostDarkMode* dm;
};

INT_PTR CALLBACK NumberDlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_INITDIALOG: {
        auto* ctx = reinterpret_cast<PromptCtx*>(lp);
        SetWindowLongPtrW(dlg, DWLP_USER, reinterpret_cast<LONG_PTR>(ctx));
        SetDlgItemInt(dlg, kIdValueEdit, ctx->minimum, FALSE);
        if (ctx->dm) ctx->dm->ApplyToDialog(dlg);
        HWND edit = GetDlgItem(dlg, kIdValueEdit);
        SetFocus(edit);
        SendMessageW(edit, EM_SETSEL, 0, -1);
        return FALSE;
    }
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT: {
        auto* ctx = reinterpret_cast<PromptCtx*>(GetWindowLongPtrW(dlg, DWLP_USER));
        LRESULT br;
        if (ctx && ctx->dm && ctx->dm->HandleCtlColor(msg, wp, br))
            return static_cast<INT_PTR>(br);
        return FALSE;
    }
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK) {
            auto* ctx = reinterpret_cast<PromptCtx*>(GetWindowLongPtrW(dlg, DWLP_USER));
            BOOL ok = FALSE;
            UINT v = GetDlgItemInt(dlg, kIdValueEdit, &ok, FALSE);
            UINT minimum = ctx ? ctx->minimum : 0;
            if (!ok || v < minimum) v = minimum;
            EndDialog(dlg, static_cast<INT_PTR>(v));
            return TRUE;
        }
        if (LOWORD(wp) == IDCANCEL) {
            EndDialog(dlg, 0);
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

struct DlgBuf {
    std::vector<uint8_t> b;
    void Align()          { while (b.size() & 3) b.push_back(0); }
    void W16(uint16_t v)  { b.push_back(v & 0xFF); b.push_back((v >> 8) & 0xFF); }
    void W32(uint32_t v)  { W16(v & 0xFFFF); W16((v >> 16) & 0xFFFF); }
    void Str(const wchar_t* s) { for (; *s; ++s) W16(static_cast<uint16_t>(*s)); W16(0); }
};

class Win32NumberPrompt : public cerfcore::NumberPrompt {
public:
    using cerfcore::NumberPrompt::NumberPrompt;

    uint32_t AskAtLeast(const std::string& title, const std::string& caption,
                        uint32_t minimum) override {
        const std::wstring wide_title   = cerfcore::Utf8ToWide(title.c_str());
        const std::wstring wide_caption = cerfcore::Utf8ToWide(caption.c_str());

        DlgBuf t;
        t.W32(WS_POPUP | WS_BORDER | WS_CAPTION | WS_SYSMENU |
              DS_MODALFRAME | DS_SETFONT | DS_CENTER);
        t.W32(0);
        t.W16(4);
        t.W16(0); t.W16(0);
        t.W16(210); t.W16(72);
        t.W16(0);
        t.W16(0);
        t.Str(wide_title.c_str());
        t.W16(9);
        t.Str(L"Segoe UI");

        t.Align();
        t.W32(WS_CHILD | WS_VISIBLE | SS_LEFT);
        t.W32(0);
        t.W16(7); t.W16(7); t.W16(196); t.W16(12);
        t.W16(0xFFFF);
        t.W16(0xFFFF); t.W16(0x0082);
        t.Str(wide_caption.c_str());
        t.W16(0);

        t.Align();
        t.W32(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP |
              ES_NUMBER | ES_AUTOHSCROLL);
        t.W32(0);
        t.W16(7); t.W16(24); t.W16(90); t.W16(12);
        t.W16(kIdValueEdit);
        t.W16(0xFFFF); t.W16(0x0081);
        t.W16(0);
        t.W16(0);

        t.Align();
        t.W32(WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON);
        t.W32(0);
        t.W16(96); t.W16(50); t.W16(50); t.W16(14);
        t.W16(IDOK);
        t.W16(0xFFFF); t.W16(0x0080);
        t.Str(L"OK");
        t.W16(0);

        t.Align();
        t.W32(WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON);
        t.W32(0);
        t.W16(152); t.W16(50); t.W16(50); t.W16(14);
        t.W16(IDCANCEL);
        t.W16(0xFFFF); t.W16(0x0080);
        t.Str(L"Cancel");
        t.W16(0);

        PromptCtx ctx{ minimum, &emu_.Get<HostDarkMode>() };
        const INT_PTR r = DialogBoxIndirectParamW(
            GetModuleHandleW(nullptr),
            reinterpret_cast<LPCDLGTEMPLATEW>(t.b.data()),
            emu_.Get<HostWindow>().Hwnd(), NumberDlgProc,
            reinterpret_cast<LPARAM>(&ctx));
        return r > 0 ? static_cast<uint32_t>(r) : 0;
    }
};

}

REGISTER_SERVICE_AS(Win32NumberPrompt, cerfcore::NumberPrompt);
