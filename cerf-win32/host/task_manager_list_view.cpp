#define NOMINMAX
#include "task_manager_list_view.h"
#include <cerfcore/platform/wide_text_win32.h>

#include <commctrl.h>
#include <uxtheme.h>

#include <string>

HWND TaskManagerListView::Create(HWND parent, int ctrl_id, HFONT font) {
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_LISTVIEW_CLASSES };
    InitCommonControlsEx(&icc);

    list_ = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                            WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT |
                            LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                            0, 0, 0, 0, parent, (HMENU)(INT_PTR)ctrl_id,
                            GetModuleHandleW(nullptr), nullptr);
    SendMessageW(list_, WM_SETFONT, (WPARAM)font, TRUE);
    ListView_SetExtendedListViewStyle(list_,
                                      LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);
    return list_;
}

void TaskManagerListView::ApplyDarkColors(COLORREF bg, COLORREF text) {
    ListView_SetBkColor(list_, bg);
    ListView_SetTextBkColor(list_, bg);
    ListView_SetTextColor(list_, text);
    SetWindowTheme(ListView_GetHeader(list_), L"DarkMode_ItemsView", nullptr);
    header_text_ = text;
    SetWindowLongPtrW(list_, GWLP_USERDATA, (LONG_PTR)this);
    list_base_proc_ = (WNDPROC)SetWindowLongPtrW(
        list_, GWLP_WNDPROC, (LONG_PTR)&TaskManagerListView::ListProcStatic);
}

LRESULT CALLBACK TaskManagerListView::ListProcStatic(HWND hwnd, UINT msg,
                                                     WPARAM wp, LPARAM lp) {
    auto* self = reinterpret_cast<TaskManagerListView*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!self || !self->list_base_proc_)
        return DefWindowProcW(hwnd, msg, wp, lp);
    if (msg == WM_NOTIFY) {
        auto* hdr = reinterpret_cast<NMHDR*>(lp);
        if (hdr->hwndFrom == ListView_GetHeader(hwnd) &&
            hdr->code == NM_CUSTOMDRAW) {
            auto* cd = reinterpret_cast<NMCUSTOMDRAW*>(lp);
            if (cd->dwDrawStage == CDDS_PREPAINT) return CDRF_NOTIFYITEMDRAW;
            if (cd->dwDrawStage == CDDS_ITEMPREPAINT) {
                SetTextColor(cd->hdc, self->header_text_);
                return CDRF_NEWFONT;
            }
        }
    }
    return CallWindowProcW(self->list_base_proc_, hwnd, msg, wp, lp);
}

void TaskManagerListView::Move(int x, int y, int w, int h) {
    MoveWindow(list_, x, y, w, h, TRUE);
}

void TaskManagerListView::SelectRow(int item) {
    if (item < 0) return;
    ListView_SetItemState(list_, item, LVIS_SELECTED | LVIS_FOCUSED,
                          LVIS_SELECTED | LVIS_FOCUSED);
}

void TaskManagerListView::SetColumns(
    const std::vector<cerffrontend::TaskManagerModel::Column>& columns) {
    ListView_DeleteAllItems(list_);
    HWND hdr = ListView_GetHeader(list_);
    const int n = hdr ? (int)SendMessageW(hdr, HDM_GETITEMCOUNT, 0, 0) : 0;
    for (int i = n - 1; i >= 0; --i) ListView_DeleteColumn(list_, i);
    for (size_t i = 0; i < columns.size(); ++i) {
        const std::wstring title = cerfcore::Utf8ToWide(columns[i].title);
        LVCOLUMNW c = {};
        c.mask    = LVCF_TEXT | LVCF_WIDTH;
        c.pszText = (LPWSTR)title.c_str();
        c.cx      = columns[i].width;
        ListView_InsertColumn(list_, (int)i, &c);
    }
}

void TaskManagerListView::Fill(const std::vector<cerffrontend::TaskManagerModel::Row>& rows) {
    uint32_t sel = 0;
    const bool had_sel = SelectedKey(&sel);

    SendMessageW(list_, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(list_);
    for (int i = 0; i < (int)rows.size(); ++i) {
        const auto&        row   = rows[(size_t)i];
        const std::wstring first = cerfcore::Utf8ToWide(row.cells[0].c_str());
        LVITEMW it = {};
        it.mask    = LVIF_TEXT | LVIF_PARAM;
        it.iItem   = i;
        it.pszText = (LPWSTR)first.c_str();
        it.lParam  = (LPARAM)row.key;
        ListView_InsertItem(list_, &it);
        for (size_t c = 1; c < row.cells.size(); ++c) {
            const std::wstring cell = cerfcore::Utf8ToWide(row.cells[c].c_str());
            ListView_SetItemText(list_, i, (int)c, (LPWSTR)cell.c_str());
        }
        if (had_sel && row.key == sel) SelectRow(i);
    }
    SendMessageW(list_, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(list_, nullptr, TRUE);
}

bool TaskManagerListView::SelectedKey(uint32_t* out) const {
    const int i = ListView_GetNextItem(list_, -1, LVNI_SELECTED);
    if (i < 0) return false;
    LVITEMW it = {};
    it.mask  = LVIF_PARAM;
    it.iItem = i;
    if (!ListView_GetItem(list_, &it)) return false;
    *out = (uint32_t)it.lParam;
    return true;
}
