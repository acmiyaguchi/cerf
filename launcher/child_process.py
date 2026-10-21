from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path
from typing import Dict, List

_LIBRARY_PATH = "LD_LIBRARY_PATH"
_BOOTLOADER_PREFIX = "_PYI_"


def child_environment() -> Dict[str, str]:
    env = dict(os.environ)
    if sys.platform == "win32" or not getattr(sys, "frozen", False):
        return env
    original = env.pop(_LIBRARY_PATH + "_ORIG", None)
    if original is not None:
        env[_LIBRARY_PATH] = original
    else:
        env.pop(_LIBRARY_PATH, None)
    for name in [n for n in env if n.startswith(_BOOTLOADER_PREFIX)]:
        del env[name]
    return env


def spawn_detached(argv: List[str], cwd: Path) -> subprocess.Popen:
    if sys.platform == "win32":
        return subprocess.Popen(argv, cwd=str(cwd),
                                creationflags=getattr(subprocess, "DETACHED_PROCESS", 0))
    return subprocess.Popen(argv, cwd=str(cwd), env=child_environment(),
                            start_new_session=True, stdin=subprocess.DEVNULL,
                            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
