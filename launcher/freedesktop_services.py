from __future__ import annotations

import re
import subprocess
import threading
from pathlib import Path
from typing import List, Optional

from child_process import child_environment, spawn_detached

_PORTAL_NAME = "org.freedesktop.portal.Desktop"
_PORTAL_PATH = "/org/freedesktop/portal/desktop"
_SETTINGS = "org.freedesktop.portal.Settings"
_APPEARANCE = "org.freedesktop.appearance"
_COLOR_SCHEME = "color-scheme"
COLOR_SCHEME_PREFER_DARK = 1

_FILE_MANAGER_NAME = "org.freedesktop.FileManager1"
_FILE_MANAGER_PATH = "/org/freedesktop/FileManager1"

_CALL_TIMEOUT_S = 3
_UINT32 = re.compile(r"uint32\s+(\d+)")


def _gdbus_call(dest: str, path: str, method: str, args: List[str]) -> Optional[str]:
    argv = ["gdbus", "call", "--session", "--dest", dest, "--object-path", path,
            "--method", method] + args
    try:
        done = subprocess.run(argv, env=child_environment(), stdin=subprocess.DEVNULL,
                              stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                              timeout=_CALL_TIMEOUT_S, check=False)
    except (OSError, subprocess.TimeoutExpired):
        return None
    if done.returncode != 0:
        return None
    return done.stdout.decode("utf-8", errors="replace")


def portal_color_scheme() -> int:
    for method in ("ReadOne", "Read"):
        reply = _gdbus_call(_PORTAL_NAME, _PORTAL_PATH, _SETTINGS + "." + method,
                            [_APPEARANCE, _COLOR_SCHEME])
        if reply is not None:
            match = _UINT32.search(reply)
            return int(match.group(1)) if match else 0
    return 0


def show_item(path: Path) -> None:
    uri = path.absolute().as_uri()
    reply = _gdbus_call(_FILE_MANAGER_NAME, _FILE_MANAGER_PATH,
                        _FILE_MANAGER_NAME + ".ShowItems", ["['{}']".format(uri), "''"])
    if reply is None:
        spawn_detached(["xdg-open", str(path.parent)], path.parent)


class AppearanceMonitor:
    def __init__(self) -> None:
        self._changed = threading.Event()
        self._proc: Optional[subprocess.Popen] = None

    def start(self) -> None:
        argv = ["gdbus", "monitor", "--session", "--dest", _PORTAL_NAME,
                "--object-path", _PORTAL_PATH]
        try:
            self._proc = subprocess.Popen(argv, env=child_environment(),
                                          stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                          stderr=subprocess.DEVNULL)
        except OSError:
            self._proc = None
            return
        threading.Thread(target=self._read, args=(self._proc,), daemon=True).start()

    def _read(self, proc: subprocess.Popen) -> None:
        for raw in proc.stdout:
            line = raw.decode("utf-8", errors="replace")
            if "SettingChanged" in line and _APPEARANCE in line:
                self._changed.set()

    def take_change(self) -> bool:
        if not self._changed.is_set():
            return False
        self._changed.clear()
        return True

    def stop(self) -> None:
        proc, self._proc = self._proc, None
        if proc is None:
            return
        proc.terminate()
        try:
            proc.wait(timeout=_CALL_TIMEOUT_S)
        except subprocess.TimeoutExpired:
            proc.kill()
