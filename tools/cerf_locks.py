from __future__ import annotations

import os
import re
import threading
import time
from dataclasses import dataclass, field
from datetime import datetime, timedelta, timezone
from pathlib import Path
from typing import List, Optional

RUN_SLOTS = 4
RUN_LOCK_STALE_SECONDS = 120
BUILD_LOCK_STALE_SECONDS = 300

_STAMP_RE = re.compile(
    r"^(\d{4})-(\d\d)-(\d\d)T(\d\d):(\d\d):(\d\d)(?:\.(\d+))?(Z|[+-]\d\d:\d\d)?$")


@dataclass
class LockState:
    stamp_utc: datetime
    owner_pid: int
    age_seconds: int


@dataclass
class CerfLock:
    path: Path
    stale_seconds: int
    label: str
    poll_seconds: int = 10
    heartbeat_seconds: int = 15
    held: bool = False
    _stop: Optional[threading.Event] = field(default=None, repr=False)
    _thread: Optional[threading.Thread] = field(default=None, repr=False)


def build_lock(root: Path, label: str) -> CerfLock:
    return CerfLock(root / ".build_lock", BUILD_LOCK_STALE_SECONDS, label)


def run_locks(root: Path, label: str) -> List[CerfLock]:
    return [CerfLock(root / ".cerf_lock.{}".format(slot), RUN_LOCK_STALE_SECONDS, label)
            for slot in range(1, RUN_SLOTS + 1)]


def _format_stamp(now: datetime) -> str:
    return now.strftime("%Y-%m-%dT%H:%M:%S.") + "{:06d}0Z".format(now.microsecond)


def _parse_stamp(text: str) -> Optional[datetime]:
    match = _STAMP_RE.match(text.strip())
    if match is None:
        return None
    year, month, day, hour, minute, second, fraction, zone = match.groups()
    micro = int((fraction or "0")[:6].ljust(6, "0"))
    try:
        stamp = datetime(int(year), int(month), int(day), int(hour), int(minute), int(second),
                         micro)
    except ValueError:
        return None
    if zone is None:
        return stamp.astimezone(timezone.utc)
    if zone == "Z":
        return stamp.replace(tzinfo=timezone.utc)
    sign = 1 if zone[0] == "+" else -1
    offset = timedelta(hours=int(zone[1:3]), minutes=int(zone[4:6]))
    return stamp.replace(tzinfo=timezone(sign * offset)).astimezone(timezone.utc)


def read_state(lock: CerfLock) -> Optional[LockState]:
    try:
        lines = lock.path.read_text(encoding="utf-8-sig").splitlines()
    except OSError:
        return None
    if not lines:
        return None
    stamp = _parse_stamp(lines[0])
    if stamp is None:
        return None
    owner = 0
    if len(lines) >= 2:
        try:
            owner = int(lines[1].strip())
        except ValueError:
            owner = 0
    age = int((datetime.now(timezone.utc) - stamp).total_seconds())
    return LockState(stamp, owner, age)


def _write_stamp(path: Path) -> bool:
    try:
        path.write_text(_format_stamp(datetime.now(timezone.utc)) + "\n" + str(os.getpid()) + "\n",
                        encoding="utf-8")
        return True
    except OSError:
        return False


def update_stamp(lock: CerfLock) -> None:
    if not lock.held:
        return
    for _ in range(5):
        if _write_stamp(lock.path):
            return
        time.sleep(0.2)


def _heartbeat(lock: CerfLock, stop: threading.Event) -> None:
    me = str(os.getpid())
    while not stop.wait(lock.heartbeat_seconds):
        try:
            lines = lock.path.read_text(encoding="utf-8-sig").splitlines()
        except FileNotFoundError:
            return
        except OSError:
            continue
        if len(lines) >= 2:
            if lines[1].strip() != me:
                return
            _write_stamp(lock.path)


def _start_heartbeat(lock: CerfLock) -> None:
    lock._stop = threading.Event()
    lock._thread = threading.Thread(target=_heartbeat, args=(lock, lock._stop), daemon=True)
    lock._thread.start()


def holder(lock: CerfLock) -> Optional[LockState]:
    if not lock.path.exists():
        return None
    state = read_state(lock)
    if state is not None and state.age_seconds < lock.stale_seconds:
        return state
    if state is not None:
        print("[{}] removing stale {} (owner PID {}, refreshed {} s ago, stale at {} s)".format(
            lock.label, lock.path.name, state.owner_pid, state.age_seconds, lock.stale_seconds),
            flush=True)
    else:
        print("[{}] removing unreadable {}".format(lock.label, lock.path.name), flush=True)
    try:
        lock.path.unlink()
    except OSError:
        pass
    if lock.path.exists():
        return LockState(datetime.now(timezone.utc), 0, 0)
    return None


def _print_waiting(lock: CerfLock, state: LockState) -> None:
    print("[{}] waiting for {} held by PID {} (refreshed {} s ago, stale at {} s)...".format(
        lock.label, lock.path.name, state.owner_pid, state.age_seconds, lock.stale_seconds),
        flush=True)


def wait(lock: CerfLock) -> None:
    while True:
        state = holder(lock)
        if state is None:
            return
        _print_waiting(lock, state)
        time.sleep(lock.poll_seconds)


def enter_no_wait(lock: CerfLock) -> bool:
    try:
        fd = os.open(str(lock.path), os.O_CREAT | os.O_EXCL | os.O_WRONLY)
    except (FileExistsError, PermissionError):
        return False
    os.close(fd)
    lock.held = True
    update_stamp(lock)
    _start_heartbeat(lock)
    return True


def enter(lock: CerfLock) -> None:
    while not enter_no_wait(lock):
        state = holder(lock)
        if state is not None:
            _print_waiting(lock, state)
            time.sleep(lock.poll_seconds)


def enter_slot(locks: List[CerfLock]) -> CerfLock:
    while True:
        for lock in locks:
            if enter_no_wait(lock):
                return lock
        held = []
        for lock in locks:
            state = holder(lock)
            if state is not None:
                held.append("{} PID {}".format(lock.path.name, state.owner_pid))
        if len(held) < len(locks):
            continue
        print("[{}] waiting for a free run slot, all {} are held: {}...".format(
            locks[0].label, len(locks), ", ".join(held)), flush=True)
        time.sleep(locks[0].poll_seconds)


def exit_lock(lock: CerfLock) -> None:
    if lock._stop is not None:
        lock._stop.set()
        if lock._thread is not None:
            lock._thread.join()
        lock._stop = None
        lock._thread = None
    if not lock.held:
        return
    lock.held = False
    state = read_state(lock)
    if state is not None and state.owner_pid != os.getpid():
        return
    try:
        lock.path.unlink()
    except OSError:
        pass


def exit_slots(locks: List[CerfLock]) -> None:
    for lock in locks:
        exit_lock(lock)
