from __future__ import annotations

import json
import platform
import sys
from dataclasses import dataclass
from pathlib import Path

KEY = "host_branch"
WIN32_X86 = "win32-x86"
LINUX_QT_AMD64 = "linux-qt-amd64"


@dataclass(frozen=True)
class HostBranch:
    name: str
    platform_dir: str
    frontend: str


_BRANCHES = {
    WIN32_X86: HostBranch(WIN32_X86, "Win32", "cerf.exe"),
    LINUX_QT_AMD64: HostBranch(LINUX_QT_AMD64, LINUX_QT_AMD64, "cerf-qt"),
}


class UnsupportedHost(Exception):
    pass


def by_name(name: str) -> HostBranch:
    branch = _BRANCHES.get(name)
    if branch is None:
        raise UnsupportedHost("unknown CERF host branch {!r}".format(name))
    return branch


def from_config(config: dict) -> HostBranch:
    name = config.get(KEY)
    if name is None:
        if sys.platform == "win32":
            return _BRANCHES[WIN32_X86]
        raise UnsupportedHost("cerf.json names no {}".format(KEY))
    if not isinstance(name, str):
        raise UnsupportedHost("cerf.json {} is not a string".format(KEY))
    return by_name(name)


def release_marker(branch: HostBranch) -> str:
    return "Release-" + branch.platform_dir


def current() -> HostBranch:
    machine = platform.machine().lower()
    if sys.platform == "win32":
        return _BRANCHES[WIN32_X86]
    if sys.platform.startswith("linux") and machine in ("x86_64", "amd64"):
        return _BRANCHES[LINUX_QT_AMD64]
    raise UnsupportedHost("no CERF host branch for {} on {}".format(sys.platform, machine))


def output_dir(root: Path, config: str, branch: HostBranch) -> Path:
    return root / "build" / config / branch.platform_dir


def stamp(cerf_json: Path, branch: HostBranch) -> None:
    data = json.loads(cerf_json.read_text(encoding="utf-8-sig")) if cerf_json.is_file() else {}
    if not isinstance(data, dict):
        raise ValueError("{} does not hold a JSON object".format(cerf_json))
    if data.get(KEY) == branch.name:
        return
    data[KEY] = branch.name
    cerf_json.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
