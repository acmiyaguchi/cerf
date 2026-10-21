#define NOMINMAX
#include "shutdown_dialog.h"

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/platform/wide_text_win32.h>
#include "dialog_band.h"
#include <cerffrontend/shutdown_prompt.h>
#include "host_dark_mode.h"
#include "host_dpi.h"
#include "host_focus_policy.h"
#include "host_window.h"
#include "win32_painter.h"

#include <commctrl.h>
#include <string>

REGISTER_SERVICE(ShutdownDialog);

namespace {
using Layout = cerffrontend::ShutdownDialogLayout;

constexpr wchar_t kClass[] = L"CerfShutdownDlg";

constexpr UINT kTimerId = 1;
constexpr int  kTotalMs = static_cast<int>(cerffrontend::ShutdownPrompt::kCountdownMs);

enum : int { IDC_ACTION = 3000, IDC_SAVE, IDC_SAVE_LINK, IDC_REMEMBER, IDC_DESC };

RECT Win32Rect(const cerffrontend::Rect& r) { return { r.left, r.top, r.right, r.bottom }; }

std::wstring StaticText(const char* utf8) {
    const std::wstring text = cerfcore::Utf8ToWide(utf8);
    std::wstring       out;
    for (const wchar_t c : text) {
        if (c == L'\n') out += L'\r';
        out += c;
    }
    return out;
}
}

std::wstring ShutdownDialog::ResetText(int action) const {
    return StaticText(emu_.Get<cerffrontend::ShutdownPrompt>().ResetDescription(
        static_cast<cerffrontend::ShutdownPromptAction>(action)));
}

int ShutdownDialog::SelectedAction() const {
    const LRESULT sel = SendMessageW(combo_, CB_GETCURSEL, 0, 0);
    return sel == CB_ERR ? 0 : (int)sel;
}

cerffrontend::ShutdownDialogLayout ShutdownDialog::Geometry() const {
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    return Layout((int)Dpi(), band_h_, rc.right, rc.bottom);
}

void ShutdownDialog::StopTimer() {
    if (!timer_on_) return;
    KillTimer(Hwnd(), kTimerId);
    timer_on_ = false;
    InvalidateRect(Hwnd(), nullptr, TRUE);
}

void ShutdownDialog::SyncActionBlock() {
    const int action = SelectedAction();
    const bool exiting = (action == 0);
    ShowWindow(chk_save_, exiting ? SW_SHOW : SW_HIDE);
    ShowWindow(save_link_, exiting ? SW_SHOW : SW_HIDE);
    ShowWindow(save_tail_, exiting ? SW_SHOW : SW_HIDE);
    ShowWindow(chk_remember_, exiting ? SW_SHOW : SW_HIDE);
    ShowWindow(desc_, exiting ? SW_HIDE : SW_SHOW);
    if (!exiting)
        SetWindowTextW(desc_, ResetText(action).c_str());
    InvalidateRect(Hwnd(), nullptr, TRUE);
}

void ShutdownDialog::CommitOk() {
    const bool save     = SendMessageW(chk_save_, BM_GETCHECK, 0, 0) == BST_CHECKED;
    const bool remember = SendMessageW(chk_remember_, BM_GETCHECK, 0, 0) == BST_CHECKED;
    choice_ = emu_.Get<cerffrontend::ShutdownPrompt>().Commit(
        static_cast<cerffrontend::ShutdownPromptAction>(SelectedAction()), save, remember);
    Finish();
}

void ShutdownDialog::BuildControls(HWND hwnd) {
    HINSTANCE inst = GetModuleHandleW(nullptr);
    const Layout g = Geometry();

    auto mk = [&](const wchar_t* cls, const wchar_t* text, DWORD style,
                  const cerffrontend::Rect& r, int extra_h, int id) {
        return CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style,
                               r.left, r.top, r.right - r.left, r.bottom - r.top + extra_h,
                               hwnd, (HMENU)(INT_PTR)id, inst, nullptr);
    };

    auto& prompt = emu_.Get<cerffrontend::ShutdownPrompt>();
    mk(L"STATIC", cerfcore::Utf8ToWide(prompt.Intro(trigger_)).c_str(), SS_LEFT, g.Intro(), 0,
       -1);

    mk(L"STATIC", cerfcore::Utf8ToWide(prompt.Question()).c_str(), SS_LEFT, g.Question(), 0, -1);

    combo_ = mk(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP | WS_VSCROLL, g.Combo(),
                Layout::kComboDropRows * g.S(Layout::kRowH), IDC_ACTION);
    for (int i = 0; i < cerffrontend::ShutdownPrompt::kActionCount; ++i) {
        const std::wstring label = cerfcore::Utf8ToWide(
            prompt.ActionLabel(static_cast<cerffrontend::ShutdownPromptAction>(i)));
        SendMessageW(combo_, CB_ADDSTRING, 0, (LPARAM)label.c_str());
    }
    SendMessageW(combo_, CB_SETCURSEL, 0, 0);

    const cerffrontend::Rect save_row = g.SaveRow();
    const cerffrontend::Rect row_cell{save_row.left, save_row.top,
                                      save_row.left + g.S(Layout::kRowH), save_row.bottom};
    chk_save_ = mk(L"BUTTON", cerfcore::Utf8ToWide(prompt.SaveLabel().c_str()).c_str(),
                   BS_AUTOCHECKBOX | WS_TABSTOP, save_row, 0, IDC_SAVE);
    SendMessageW(chk_save_, BM_SETCHECK, prompt.SaveByDefault() ? BST_CHECKED : BST_UNCHECKED,
                 0);

    const std::wstring link = L"<a href=\"" + cerfcore::Utf8ToWide(prompt.SaveLinkUrl()) +
                              L"\">" + cerfcore::Utf8ToWide(prompt.SaveLinkText()) + L"</a>";
    save_link_ = mk(WC_LINK, link.c_str(), WS_TABSTOP, row_cell, 0, IDC_SAVE_LINK);
    save_tail_text_ = cerfcore::Utf8ToWide(prompt.SaveTail());
    save_tail_ = mk(L"STATIC", save_tail_text_.c_str(), SS_LEFT, row_cell, 0, -1);

    chk_remember_ = mk(L"BUTTON", cerfcore::Utf8ToWide(prompt.RememberLabel()).c_str(),
                       BS_AUTOCHECKBOX | WS_TABSTOP, g.Remember(), 0, IDC_REMEMBER);

    const std::wstring soft =
        ResetText(static_cast<int>(cerffrontend::ShutdownPromptAction::SoftReset));
    desc_ = mk(L"STATIC", soft.c_str(), SS_LEFT, g.Description(), 0, IDC_DESC);
    ShowWindow(desc_, SW_HIDE);

    mk(L"BUTTON", L"OK", BS_DEFPUSHBUTTON | WS_TABSTOP, g.Ok(), 0, IDOK);
    mk(L"BUTTON", cerfcore::Utf8ToWide(prompt.CancelLabel(trigger_)).c_str(),
       BS_PUSHBUTTON | WS_TABSTOP, g.Cancel(), 0, IDCANCEL);

    if (timer_on_) {
        deadline_tick_ = GetTickCount64() + kTotalMs;
        SetTimer(hwnd, kTimerId, Layout::kTickMs, nullptr);
    }
}

void ShutdownDialog::LayoutSaveRow() {
    const cerffrontend::Rect save_row = Geometry().SaveRow();
    const int block_y = save_row.top;
    const int row_h   = save_row.bottom - save_row.top;

    SIZE chk = {};
    SendMessageW(chk_save_, BCM_GETIDEALSIZE, 0, (LPARAM)&chk);
    SetWindowPos(chk_save_, nullptr, save_row.left, block_y, chk.cx, row_h,
                 SWP_NOZORDER | SWP_NOACTIVATE);

    const int link_x = save_row.left + chk.cx;
    SIZE link = {};
    SendMessageW(save_link_, LM_GETIDEALSIZE, save_row.right - link_x, (LPARAM)&link);
    const int link_y = block_y + (row_h - link.cy) / 2;
    SetWindowPos(save_link_, nullptr, link_x, link_y, link.cx, link.cy,
                 SWP_NOZORDER | SWP_NOACTIVATE);

    SIZE tail = {};
    HDC dc = GetDC(save_tail_);
    HGDIOBJ old = SelectObject(dc, (HFONT)SendMessageW(save_tail_, WM_GETFONT, 0, 0));
    GetTextExtentPoint32W(dc, save_tail_text_.c_str(), static_cast<int>(save_tail_text_.size()),
                          &tail);
    SelectObject(dc, old);
    ReleaseDC(save_tail_, dc);
    SetWindowPos(save_tail_, nullptr, link_x + link.cx, link_y, tail.cx,
                 link.cy, SWP_NOZORDER | SWP_NOACTIVATE);
}

void ShutdownDialog::OnShown() {
    LayoutSaveRow();
    emu_.Get<HostFocusPolicy>().Focus(combo_);
}

void ShutdownDialog::OnPaint(HDC dc) {
    auto& dm = emu_.Get<HostDarkMode>();
    const bool dark = dm.IsDark();

    emu_.Get<DialogBand>().Paint(dc, Dpi(), 0, 0);

    const Layout             g    = Geometry();
    const cerffrontend::Rect icon = g.Icon();
    if (HICON h = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(2)))
        DrawIconEx(dc, icon.left, icon.top, h, icon.right - icon.left, icon.bottom - icon.top, 0,
                   nullptr, DI_NORMAL);

    if (!timer_on_ || SelectedAction() != 0) return;

    painted_fill_w_ = g.BarFillWidth(remaining_ms_);
    painted_secs_   = Layout::RemainingSeconds(remaining_ms_);
    const COLORREF bg = dark ? dm.BgColor() : GetSysColor(COLOR_BTNFACE);
    Win32Painter   painter(emu_, dc);
    g.PaintCountdown(painter, cerffrontend::Rgb{GetRValue(bg), GetGValue(bg), GetBValue(bg)}, dark,
                     remaining_ms_);
}

void ShutdownDialog::OnCommand(int id, int notify) {
    if (id == IDOK) {
        CommitOk();
        return;
    }
    if (id == IDCANCEL) {
        choice_ = cerfcore::ShutdownChoice::Cancel;
        Finish();
        return;
    }
    if (id == IDC_ACTION) {
        if (notify == CBN_DROPDOWN) StopTimer();
        if (notify == CBN_SELCHANGE) {
            StopTimer();
            SyncActionBlock();
        }
        return;
    }
    if ((id == IDC_SAVE || id == IDC_REMEMBER) && notify == BN_CLICKED)
        StopTimer();
}

bool ShutdownDialog::OnMessage(UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_LBUTTONDOWN) {
        StopTimer();
        return true;
    }
    if (msg == WM_NOTIFY) {
        const auto* nh = reinterpret_cast<const NMHDR*>(lp);
        if (nh->hwndFrom == save_link_ &&
            (nh->code == NM_CLICK || nh->code == NM_RETURN))
            StopTimer();
        return false;
    }
    if (msg != WM_TIMER || wp != kTimerId) return false;

    const ULONGLONG now = GetTickCount64();
    remaining_ms_ = (now >= deadline_tick_)
        ? 0 : (int)(deadline_tick_ - now);
    if (remaining_ms_ <= 0) {
        KillTimer(Hwnd(), kTimerId);
        timer_on_ = false;
        CommitOk();
        return true;
    }
    const Layout g = Geometry();
    if (g.BarFillWidth(remaining_ms_) != painted_fill_w_) {
        const RECT bar = Win32Rect(g.Bar());
        InvalidateRect(Hwnd(), &bar, FALSE);
    }
    if (Layout::RemainingSeconds(remaining_ms_) != painted_secs_) {
        const RECT crc = Win32Rect(g.Countdown());
        InvalidateRect(Hwnd(), &crc, FALSE);
    }
    return true;
}

void ShutdownDialog::DismissAsCancel() {
    PostDismiss();
}

cerfcore::ShutdownChoice ShutdownDialog::Show(cerfcore::ShutdownTrigger trigger) {
    if (IsOpen()) return cerfcore::ShutdownChoice::Cancel;

    HWND owner = emu_.Get<HostWindow>().Hwnd();
    trigger_        = trigger;
    choice_         = cerfcore::ShutdownChoice::Cancel;
    timer_on_       = emu_.Get<cerffrontend::ShutdownPrompt>().CountsDown(trigger);
    remaining_ms_   = kTotalMs;
    painted_fill_w_ = -1;
    painted_secs_   = -1;

    const UINT dpi = emu_.Get<HostDpi>().ForWindow(owner);
    auto& band = emu_.Get<DialogBand>();
    band_h_ = band.PixelHeight(dpi);
    const int client_w = band.PixelWidth(dpi);
    const int client_h = Layout::ClientHeight((int)dpi, band_h_);

    RunModal(owner, kClass,
             cerfcore::Utf8ToWide(emu_.Get<cerffrontend::ShutdownPrompt>().Title()).c_str(),
             client_w, client_h);
    return choice_;
}
