from __future__ import annotations

import shutil
import subprocess
from datetime import datetime
from pathlib import Path
from typing import Callable, List, Optional

from host_branch import HostBranch, output_dir


class BuildAbort(Exception):
    pass


def _run(argv: List[str]) -> int:
    return subprocess.run([str(a) for a in argv], check=False).returncode


class LinuxBuild:
    def __init__(self, root: Path, config: str, mode: str, rebuild: bool,
                 heartbeat: Callable[[], None], branch: HostBranch) -> None:
        self.root = root
        self.config = config
        self.mode = mode
        self.rebuild = rebuild
        self.heartbeat = heartbeat
        self.branch = branch
        self.cmake: Optional[str] = None
        self.ccache: Optional[str] = None

    def discover(self) -> None:
        missing = [tool for tool in ("cmake", "ninja", "g++") if shutil.which(tool) is None]
        if missing:
            raise BuildAbort("[BUILD] FAILED! {} not found on PATH. Install CMake, Ninja, GCC and "
                             "the Qt 6 development packages.".format(", ".join(missing)))
        self.cmake = shutil.which("cmake")
        self.ccache = shutil.which("ccache")
        print("[BUILD] cmake {}, ninja {}, g++ {}{}".format(
            self.cmake, shutil.which("ninja"), shutil.which("g++"),
            ", ccache {}".format(self.ccache) if self.ccache else ""), flush=True)

    def build(self) -> bool:
        cmake_dir = self.root / "build" / "cmake-{}-{}".format(self.branch.name,
                                                               self.config.lower())
        out = output_dir(self.root, self.config, self.branch)
        dev = "OFF" if self.mode == "production" else "ON"
        if self.rebuild and cmake_dir.exists():
            shutil.rmtree(cmake_dir)
        configure = [self.cmake, "-S", self.root, "-B", cmake_dir, "-G", "Ninja",
                     "-DCMAKE_BUILD_TYPE=" + self.config, "-DCMAKE_CXX_COMPILER=g++",
                     "-DCERF_BUILD_QT=ON", "-DCERFCORE_DEV_MODE=" + dev,
                     "-DCERFCORE_DEVICE_TRACES=" + dev,
                     "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY={}".format(out),
                     "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"]
        if self.ccache:
            configure.append("-DCMAKE_CXX_COMPILER_LAUNCHER=" + self.ccache)
        print("[LIBS] libCerfEngine, libCerfCore, libCerfFrontend, cerf-qt ({})".format(
            self.config), flush=True)
        code = _run(configure)
        self.heartbeat()
        if code != 0:
            raise BuildAbort("[BUILD] CMake configure FAILED (exit={})".format(code))
        code = _run([self.cmake, "--build", cmake_dir, "--parallel"])
        self.heartbeat()
        exe = out / self.branch.frontend
        if code != 0:
            print("[BUILD] {} FAILED (cmake --build exit={})".format(self.branch.frontend, code),
                  flush=True)
            return False
        if not exe.is_file():
            print("[BUILD] {} FAILED (the build reported success but the binary is not at {})"
                  .format(self.branch.frontend, exe), flush=True)
            return False
        stat = exe.stat()
        print("[BUILD] {} OK: {}".format(self.branch.frontend, exe), flush=True)
        print("[BUILD] Size: {} bytes".format(stat.st_size), flush=True)
        print("[BUILD] Time: {}".format(datetime.fromtimestamp(stat.st_mtime)), flush=True)
        return True
