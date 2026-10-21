from __future__ import annotations

import sys
from pathlib import Path

from app_paths import FRONTEND_NAME
from device_state import RunningStatus

if sys.platform == "win32":
    import ctypes
    from ctypes import wintypes

    _user32 = ctypes.windll.user32

    SW_RESTORE = 9

    def _belongs_to_pid(hwnd: int, pid: int) -> bool:
        out = wintypes.DWORD(0)
        _user32.GetWindowThreadProcessId(wintypes.HWND(hwnd), ctypes.byref(out))
        return out.value == pid

    def show_running_window(status: RunningStatus) -> bool:
        hwnd = status.hwnd
        if not hwnd or not _user32.IsWindow(wintypes.HWND(hwnd)):
            return False
        if status.pid and not _belongs_to_pid(hwnd, status.pid):
            return False
        _user32.ShowWindow(wintypes.HWND(hwnd), SW_RESTORE)
        _user32.SetForegroundWindow(wintypes.HWND(hwnd))
        return True
else:
    import socket

    def _is_frontend(pid: int) -> bool:
        try:
            comm = Path("/proc/{}/comm".format(pid)).read_text(encoding="utf-8").strip()
        except OSError:
            return False
        return comm == FRONTEND_NAME

    def show_running_window(status: RunningStatus) -> bool:
        if not status.pid or not _is_frontend(status.pid):
            return False
        address = b"\0" + "cerf-show/{}".format(status.pid).encode("ascii")
        try:
            with socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM) as s:
                s.sendto(b"show", address)
        except OSError:
            return False
        return True
