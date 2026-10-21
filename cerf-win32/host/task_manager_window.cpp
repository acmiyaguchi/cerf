#define NOMINMAX
#include <cerfcore/host/task_manager_view.h>
#include "task_manager_list_view.h"

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/device_config.h>
#include <cerfcore/peripherals/cerf_virt/cerf_virt_task_manager_regs.h>
#include "host_dark_mode.h"
#include "host_window.h"
#include <cerfcore/platform/wide_text_win32.h>
#include <cerffrontend/theme_colors.h>

#include <cstdint>
#include <string>

namespace {

using Model = cerffrontend::TaskManagerModel;

constexpr wchar_t kWndClass[] = L"CerfTaskManagerWnd";

constexpr UINT kRefreshTimerId = 1;

constexpr int kMargin     = Model::kMargin;
constexpr int kBottomBarH = Model::kBottomBarH;
constexpr int kRowH       = Model::kRowH;
constexpr int kBtnW       = Model::kBtnW;

enum : int {
    IDC_TABS    = 2000,
    IDC_LIST    = 2001,
    IDC_RUNEDIT = 2002,
    IDC_RUN     = 2003,
    IDC_SWITCH  = 2004,
    IDC_KILL    = 2005,
    IDM_ROW_SWITCH = 2101,
    IDM_ROW_KILL   = 2102,
};

constexpr int kTabWindows   = 0;
constexpr int kTabProcesses = 1;

constexpr COLORREF Colorref(cerffrontend::Rgb c) { return RGB(c.r, c.g, c.b); }

constexpr COLORREF kTabStripBg = Colorref(cerffrontend::theme::kDarkBar);
constexpr COLORREF kTabSelBg   = Colorref(cerffrontend::theme::kDarkHot);
constexpr COLORREF kTabText    = Colorref(cerffrontend::theme::kDarkText);

COLORREF StatusColor(Model::StatusKind kind) {
    switch (kind) {
        case Model::StatusKind::kOk:   return Colorref(Model::kOkColor);
        case Model::StatusKind::kFail: return Colorref(Model::kFailColor);
        case Model::StatusKind::kInfo: return Colorref(Model::kInfoColor);
    }
    return Colorref(Model::kInfoColor);
}

class TaskManagerWindow : public cerfcore::TaskManagerView {
public:
    using cerfcore::TaskManagerView::TaskManagerView;

    bool ShouldRegister() override;
    void OnReady() override;

    void Show() override;

private:
    static LRESULT CALLBACK WndProcStatic(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK RunEditProcStatic(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK TabProcStatic(HWND, UINT, WPARAM, LPARAM);
    LRESULT WndProc(HWND, UINT, WPARAM, LPARAM);
    void    DrawTab(const DRAWITEMSTRUCT* di);

    void BuildControls();
    void LayoutControls();
    void OnTick();
    void OnTabChanged();
    void DoSwitchTo();
    void DoKill();
    void DoRun();
    void ShowRowMenu(int item, POINT screen_pt);
    void SyncStatus();

    HWND                hwnd_       = nullptr;
    HWND                tabs_       = nullptr;
    TaskManagerListView list_;
    HWND                run_label_  = nullptr;
    HWND                run_edit_   = nullptr;
    HWND                btn_run_    = nullptr;
    HWND                btn_switch_ = nullptr;
    HWND                btn_kill_   = nullptr;
    HWND                status_     = nullptr;
    WNDPROC             run_edit_base_proc_ = nullptr;
    WNDPROC             tabs_base_proc_     = nullptr;

    uint64_t status_serial_ = 0;
    COLORREF status_clr_    = RGB(0, 0, 0);
};

bool TaskManagerWindow::ShouldRegister() {
    return emu_.Get<cerfcore::DeviceConfig>().guest_additions;
}

void TaskManagerWindow::OnReady() {
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = &TaskManagerWindow::WndProcStatic;
    wc.hInstance     = GetModuleHandleW(nullptr);
    wc.hIcon         = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(1));
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = kWndClass;
    RegisterClassExW(&wc);   /* ERROR_CLASS_ALREADY_EXISTS is benign */
}

void TaskManagerWindow::Show() {
    if (hwnd_) {
        if (IsIconic(hwnd_)) ShowWindow(hwnd_, SW_RESTORE);
        SetForegroundWindow(hwnd_);
        return;
    }

    HWND owner = emu_.Get<HostWindow>().Hwnd();
    const DWORD style = WS_OVERLAPPEDWINDOW;
    RECT wr = { 0, 0, Model::kClientW, Model::kClientH };
    AdjustWindowRectEx(&wr, style, FALSE, 0);
    hwnd_ = CreateWindowExW(0, kWndClass, L"Guest Task Manager", style,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            wr.right - wr.left, wr.bottom - wr.top,
                            owner, nullptr, GetModuleHandleW(nullptr), this);
    if (!hwnd_) return;

    BuildControls();
    LayoutControls();

    auto& dm = emu_.Get<HostDarkMode>();
    dm.ApplyToDialog(hwnd_);
    if (dm.IsDark()) list_.ApplyDarkColors(dm.BgColor(), dm.TextColor());

    ShowWindow(hwnd_, SW_SHOW);
    SetTimer(hwnd_, kRefreshTimerId, Model::kRefreshMs, nullptr);

    auto& model = emu_.Get<Model>();
    model.Open();
    list_.SetColumns(model.Columns());
    SyncStatus();
}

void TaskManagerWindow::BuildControls() {
    INITCOMMONCONTROLSEX icc = { sizeof(icc),
                                 ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES };
    InitCommonControlsEx(&icc);

    HINSTANCE inst = GetModuleHandleW(nullptr);
    HFONT gui = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    auto mk = [&](const wchar_t* cls, const wchar_t* text, DWORD style,
                  int id) {
        HWND h = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style,
                                 0, 0, 0, 0, hwnd_, (HMENU)(INT_PTR)id,
                                 inst, nullptr);
        SendMessageW(h, WM_SETFONT, (WPARAM)gui, TRUE);
        return h;
    };

    /* Tab controls have no uxtheme dark class, so in dark mode we owner-draw the
       tab buttons (WM_DRAWITEM) and dark-fill the strip behind them via a
       subclass; classic comctl would otherwise paint them light. */
    const bool dark = emu_.Get<HostDarkMode>().IsDark();
    DWORD tab_style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;
    if (dark) tab_style |= TCS_OWNERDRAWFIXED;
    tabs_ = CreateWindowExW(0, WC_TABCONTROLW, L"", tab_style,
                            0, 0, 0, 0, hwnd_, (HMENU)(INT_PTR)IDC_TABS,
                            inst, nullptr);
    SendMessageW(tabs_, WM_SETFONT, (WPARAM)gui, TRUE);
    TCITEMW ti = {};
    ti.mask    = TCIF_TEXT;
    ti.pszText = (LPWSTR)L"Windows";   TabCtrl_InsertItem(tabs_, kTabWindows, &ti);
    ti.pszText = (LPWSTR)L"Processes"; TabCtrl_InsertItem(tabs_, kTabProcesses, &ti);
    if (dark) {
        SetWindowLongPtrW(tabs_, GWLP_USERDATA, (LONG_PTR)this);
        tabs_base_proc_ = (WNDPROC)SetWindowLongPtrW(
            tabs_, GWLP_WNDPROC, (LONG_PTR)&TaskManagerWindow::TabProcStatic);
    }

    list_ = TaskManagerListView{};
    list_.Create(hwnd_, IDC_LIST, gui);

    btn_switch_ = mk(L"BUTTON", L"Switch to", BS_PUSHBUTTON | WS_TABSTOP, IDC_SWITCH);
    btn_kill_   = mk(L"BUTTON", L"Kill",      BS_PUSHBUTTON | WS_TABSTOP, IDC_KILL);
    run_label_  = mk(L"STATIC", L"Run:", 0, 0);
    run_edit_   = mk(L"EDIT", L"", WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                     IDC_RUNEDIT);
    btn_run_    = mk(L"BUTTON", L"Run", BS_DEFPUSHBUTTON | WS_TABSTOP, IDC_RUN);
    status_     = mk(L"STATIC", L"", SS_LEFTNOWORDWRAP, 0);

    SendMessageW(run_edit_, EM_SETLIMITTEXT, cerfcore::CerfVirt::kTmRunMaxWchars, 0);
    SetWindowLongPtrW(run_edit_, GWLP_USERDATA, (LONG_PTR)this);
    run_edit_base_proc_ = (WNDPROC)SetWindowLongPtrW(
        run_edit_, GWLP_WNDPROC, (LONG_PTR)&TaskManagerWindow::RunEditProcStatic);
}

void TaskManagerWindow::LayoutControls() {
    RECT rc;
    GetClientRect(hwnd_, &rc);
    const int w = rc.right, h = rc.bottom;

    const int tabs_bottom = h - kBottomBarH - kMargin;
    MoveWindow(tabs_, kMargin, kMargin, w - 2 * kMargin,
               tabs_bottom - kMargin, TRUE);

    RECT tr = { kMargin, kMargin, w - kMargin, tabs_bottom };
    TabCtrl_AdjustRect(tabs_, FALSE, &tr);   /* tab window rect -> display rect */
    list_.Move(tr.left, tr.top, tr.right - tr.left, tr.bottom - tr.top);

    const int row1 = h - kBottomBarH;
    MoveWindow(btn_switch_, kMargin, row1, kBtnW, kRowH, TRUE);
    MoveWindow(btn_kill_, kMargin + kBtnW + 6, row1, kBtnW, kRowH, TRUE);

    const int row2    = row1 + kRowH + 8;
    const int label_w = Model::kLabelW;
    const int raw_w   = w - 2 * kMargin - label_w - kBtnW - 12 - Model::kEditReserve;
    const int edit_w  = raw_w > Model::kEditMinW ? raw_w : Model::kEditMinW;
    MoveWindow(run_label_, kMargin, row2 + 4, label_w, kRowH - 6, TRUE);
    MoveWindow(run_edit_, kMargin + label_w, row2, edit_w, kRowH - 2, TRUE);
    MoveWindow(btn_run_, kMargin + label_w + edit_w + 6, row2, kBtnW, kRowH, TRUE);
    MoveWindow(status_, kMargin + label_w + edit_w + kBtnW + 12, row2 + 4, Model::kStatusW,
               kRowH - 6, TRUE);
}

void TaskManagerWindow::SyncStatus() {
    auto& model = emu_.Get<Model>();
    if (model.StatusSerial() == status_serial_) return;
    status_serial_ = model.StatusSerial();
    status_clr_    = StatusColor(model.Status());
    SetWindowTextW(status_, cerfcore::Utf8ToWide(model.StatusText().c_str()).c_str());
    InvalidateRect(status_, nullptr, TRUE);
}

void TaskManagerWindow::OnTabChanged() {
    auto& model = emu_.Get<Model>();
    model.SetMode(TabCtrl_GetCurSel(tabs_) == kTabProcesses ? Model::Mode::kProcesses
                                                            : Model::Mode::kWindows);
    list_.SetColumns(model.Columns());
    SyncStatus();
}

void TaskManagerWindow::OnTick() {
    auto& model = emu_.Get<Model>();
    if (model.Tick()) {
        list_.Fill(model.Rows());
        SetWindowTextW(hwnd_, cerfcore::Utf8ToWide(model.Title().c_str()).c_str());
    }
    SyncStatus();
}

void TaskManagerWindow::DoSwitchTo() {
    uint32_t   key      = 0;
    const bool selected = list_.SelectedKey(&key);
    emu_.Get<Model>().SwitchTo(selected, key);
    SyncStatus();
}

void TaskManagerWindow::DoKill() {
    uint32_t   key      = 0;
    const bool selected = list_.SelectedKey(&key);
    emu_.Get<Model>().Kill(selected, key);
    SyncStatus();
}

void TaskManagerWindow::DoRun() {
    wchar_t buf[cerfcore::CerfVirt::kTmRunMaxWchars + 1] = { 0 };
    GetWindowTextW(run_edit_, buf, (int)(cerfcore::CerfVirt::kTmRunMaxWchars + 1));
    emu_.Get<Model>().Run(cerfcore::WideToUtf16(buf));
    SyncStatus();
}

void TaskManagerWindow::ShowRowMenu(int item, POINT screen_pt) {
    list_.SelectRow(item);
    HMENU m = CreatePopupMenu();
    AppendMenuW(m, MF_STRING, IDM_ROW_SWITCH, L"Switch to");
    AppendMenuW(m, MF_STRING, IDM_ROW_KILL, L"Kill");
    TrackPopupMenu(m, TPM_RIGHTBUTTON, screen_pt.x, screen_pt.y, 0, hwnd_,
                   nullptr);
    DestroyMenu(m);
}

void TaskManagerWindow::DrawTab(const DRAWITEMSTRUCT* di) {
    const bool sel = (di->itemState & ODS_SELECTED) != 0;
    SetDCBrushColor(di->hDC, sel ? kTabSelBg : kTabStripBg);
    FillRect(di->hDC, &di->rcItem, (HBRUSH)GetStockObject(DC_BRUSH));

    wchar_t txt[32] = {};
    TCITEMW ti = {};
    ti.mask       = TCIF_TEXT;
    ti.pszText    = txt;
    ti.cchTextMax = 31;
    TabCtrl_GetItem(tabs_, di->itemID, &ti);

    SetTextColor(di->hDC, kTabText);
    SetBkMode(di->hDC, TRANSPARENT);
    RECT rc = di->rcItem;
    DrawTextW(di->hDC, txt, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

LRESULT CALLBACK TaskManagerWindow::RunEditProcStatic(HWND hwnd, UINT msg,
                                                      WPARAM wp, LPARAM lp) {
    auto* self = reinterpret_cast<TaskManagerWindow*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_KEYDOWN && wp == VK_RETURN) {
        PostMessageW(GetParent(hwnd), WM_COMMAND,
                     MAKEWPARAM(IDC_RUN, BN_CLICKED), 0);
        return 0;
    }
    if (msg == WM_CHAR && wp == L'\r') return 0;   /* swallow the beep */
    return CallWindowProcW(self->run_edit_base_proc_, hwnd, msg, wp, lp);
}

LRESULT CALLBACK TaskManagerWindow::TabProcStatic(HWND hwnd, UINT msg,
                                                  WPARAM wp, LPARAM lp) {
    auto* self = reinterpret_cast<TaskManagerWindow*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!self || !self->tabs_base_proc_)
        return DefWindowProcW(hwnd, msg, wp, lp);
    if (msg == WM_ERASEBKGND) {
        RECT rc;
        GetClientRect(hwnd, &rc);
        SetDCBrushColor((HDC)wp, kTabStripBg);
        FillRect((HDC)wp, &rc, (HBRUSH)GetStockObject(DC_BRUSH));
        return 1;
    }
    return CallWindowProcW(self->tabs_base_proc_, hwnd, msg, wp, lp);
}

LRESULT CALLBACK TaskManagerWindow::WndProcStatic(HWND hwnd, UINT msg,
                                                  WPARAM wp, LPARAM lp) {
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
    auto* self = reinterpret_cast<TaskManagerWindow*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!self) return DefWindowProcW(hwnd, msg, wp, lp);
    return self->WndProc(hwnd, msg, wp, lp);
}

LRESULT TaskManagerWindow::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_SIZE:
            LayoutControls();
            return 0;

        case WM_GETMINMAXINFO: {
            auto* mmi = reinterpret_cast<MINMAXINFO*>(lp);
            mmi->ptMinTrackSize = { Model::kMinTrackW, Model::kMinTrackH };
            return 0;
        }

        case WM_TIMER:
            if (wp == kRefreshTimerId) OnTick();
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case IDC_RUN:         DoRun();      return 0;
                case IDC_SWITCH:
                case IDM_ROW_SWITCH:  DoSwitchTo(); return 0;
                case IDC_KILL:
                case IDM_ROW_KILL:    DoKill();     return 0;
            }
            return 0;

        case WM_DRAWITEM: {
            auto* di = reinterpret_cast<DRAWITEMSTRUCT*>(lp);
            if (di->CtlType == ODT_TAB && di->hwndItem == tabs_) {
                DrawTab(di);
                return TRUE;
            }
            break;
        }

        case WM_NOTIFY: {
            auto* hdr = reinterpret_cast<NMHDR*>(lp);
            if (hdr->hwndFrom == tabs_ && hdr->code == TCN_SELCHANGE) {
                OnTabChanged();
                return 0;
            }
            if (hdr->hwndFrom == list_.Hwnd() && hdr->code == NM_RCLICK) {
                auto* ia = reinterpret_cast<NMITEMACTIVATE*>(lp);
                POINT pt = ia->ptAction;
                ClientToScreen(list_.Hwnd(), &pt);
                ShowRowMenu(ia->iItem, pt);
                return 0;
            }
            break;
        }

        case WM_CTLCOLORSTATIC: {
            auto& dm = emu_.Get<HostDarkMode>();
            if ((HWND)lp == status_) {
                HDC dc = (HDC)wp;
                SetTextColor(dc, status_clr_);
                SetBkMode(dc, TRANSPARENT);
                if (dm.IsDark()) {
                    SetBkColor(dc, dm.BgColor());
                    return (LRESULT)dm.BgBrush();
                }
                return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
            }
            LRESULT br;
            if (dm.HandleCtlColor(msg, wp, br)) return br;
            break;
        }

        case WM_CTLCOLORDLG:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX: {
            LRESULT br;
            if (emu_.Get<HostDarkMode>().HandleCtlColor(msg, wp, br)) return br;
            break;
        }

        case WM_ERASEBKGND:
            if (emu_.Get<HostDarkMode>().EraseBackground((HDC)wp, hwnd))
                return 1;
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            KillTimer(hwnd, kRefreshTimerId);
            hwnd_ = nullptr;
            tabs_ = run_edit_ = btn_run_ = btn_switch_ = btn_kill_ = nullptr;
            run_label_ = status_ = nullptr;
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

}

REGISTER_SERVICE_AS(TaskManagerWindow, cerfcore::TaskManagerView);
