#pragma once

#include <cerffrontend/task_manager_model.h>

#define NOMINMAX
#include <windows.h>

#include <cstdint>
#include <vector>

class TaskManagerListView {
public:
    HWND Create(HWND parent, int ctrl_id, HFONT font);
    HWND Hwnd() const { return list_; }
    void ApplyDarkColors(COLORREF bg, COLORREF text);
    void Move(int x, int y, int w, int h);
    void SelectRow(int item);

    void SetColumns(const std::vector<cerffrontend::TaskManagerModel::Column>& columns);
    void Fill(const std::vector<cerffrontend::TaskManagerModel::Row>& rows);
    bool SelectedKey(uint32_t* out) const;

private:
    static LRESULT CALLBACK ListProcStatic(HWND, UINT, WPARAM, LPARAM);

    HWND     list_           = nullptr;
    WNDPROC  list_base_proc_ = nullptr;
    COLORREF header_text_    = RGB(0, 0, 0);
};
