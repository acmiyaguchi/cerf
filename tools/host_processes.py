from __future__ import annotations

import csv
import io
import os
import signal
import subprocess
import sys
from pathlib import Path
from typing import Dict, Optional

_WINDOWS = sys.platform == "win32"

if _WINDOWS:
    import ctypes
    from ctypes import wintypes

    _PROCESS_QUERY_LIMITED_INFORMATION = 0x1000
    _PROCESS_VM_READ = 0x0010

    class _MemoryCountersEx(ctypes.Structure):
        _fields_ = [("cb", wintypes.DWORD), ("PageFaultCount", wintypes.DWORD),
                    ("PeakWorkingSetSize", ctypes.c_size_t), ("WorkingSetSize", ctypes.c_size_t),
                    ("QuotaPeakPagedPoolUsage", ctypes.c_size_t),
                    ("QuotaPagedPoolUsage", ctypes.c_size_t),
                    ("QuotaPeakNonPagedPoolUsage", ctypes.c_size_t),
                    ("QuotaNonPagedPoolUsage", ctypes.c_size_t),
                    ("PagefileUsage", ctypes.c_size_t), ("PeakPagefileUsage", ctypes.c_size_t),
                    ("PrivateUsage", ctypes.c_size_t)]

    _kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    _kernel32.OpenProcess.restype = wintypes.HANDLE
    _kernel32.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
    _kernel32.CloseHandle.argtypes = [wintypes.HANDLE]
    _kernel32.K32GetProcessMemoryInfo.argtypes = [wintypes.HANDLE,
                                                  ctypes.POINTER(_MemoryCountersEx),
                                                  wintypes.DWORD]
    _kernel32.GetProcessTimes.argtypes = [wintypes.HANDLE] + [ctypes.POINTER(wintypes.FILETIME)] * 4


def list_processes() -> Dict[int, str]:
    if _WINDOWS:
        out = subprocess.run(["tasklist", "/FO", "CSV", "/NH"], capture_output=True, text=True,
                             errors="replace", check=False).stdout
        procs = {}
        for row in csv.reader(io.StringIO(out)):
            if len(row) >= 2 and row[1].isdigit():
                name = row[0][:-4] if row[0].lower().endswith(".exe") else row[0]
                procs[int(row[1])] = name
        return procs
    procs = {}
    for entry in Path("/proc").iterdir():
        if not entry.name.isdigit():
            continue
        try:
            procs[int(entry.name)] = (entry / "comm").read_text().strip()
        except OSError:
            continue
    return procs


def _open(pid: int, access: int):
    handle = _kernel32.OpenProcess(access, False, pid)
    return handle or None


def private_bytes(pid: int) -> Optional[int]:
    if _WINDOWS:
        handle = _open(pid, _PROCESS_QUERY_LIMITED_INFORMATION | _PROCESS_VM_READ)
        if handle is None:
            return None
        try:
            counters = _MemoryCountersEx()
            counters.cb = ctypes.sizeof(counters)
            if not _kernel32.K32GetProcessMemoryInfo(handle, ctypes.byref(counters), counters.cb):
                return None
            return int(counters.PrivateUsage)
        finally:
            _kernel32.CloseHandle(handle)
    try:
        status = Path("/proc/{}/status".format(pid)).read_text()
    except OSError:
        return None
    fields = {}
    for line in status.splitlines():
        key, _, value = line.partition(":")
        parts = value.split()
        if parts and parts[0].isdigit():
            fields[key] = int(parts[0]) * 1024
    if "RssAnon" not in fields:
        return None
    return fields["RssAnon"] + fields.get("VmSwap", 0)


def start_time(pid: int) -> Optional[int]:
    if _WINDOWS:
        handle = _open(pid, _PROCESS_QUERY_LIMITED_INFORMATION)
        if handle is None:
            return None
        try:
            times = [wintypes.FILETIME() for _ in range(4)]
            if not _kernel32.GetProcessTimes(handle, *[ctypes.byref(t) for t in times]):
                return None
            return (times[0].dwHighDateTime << 32) | times[0].dwLowDateTime
        finally:
            _kernel32.CloseHandle(handle)
    try:
        stat = Path("/proc/{}/stat".format(pid)).read_text()
    except OSError:
        return None
    return int(stat[stat.rfind(")") + 2:].split()[19])


def kill(pid: int) -> None:
    if _WINDOWS:
        subprocess.run(["taskkill", "/F", "/PID", str(pid)], capture_output=True, check=False)
        return
    try:
        os.kill(pid, signal.SIGKILL)
    except OSError:
        pass
