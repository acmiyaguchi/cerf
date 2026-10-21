from __future__ import annotations

import json
import os
import re
import shutil
import subprocess
from datetime import datetime
from pathlib import Path
from typing import Callable, List

import vs_toolchain

VCPKG_TRIPLET = "x86-windows-static"
GENERATOR_KEYS = ("CMAKE_GENERATOR", "CMAKE_GENERATOR_PLATFORM", "CMAKE_GENERATOR_TOOLSET",
                  "CMAKE_GENERATOR_INSTANCE")


class BuildAbort(Exception):
    pass


def vcpkg_props_path() -> Path:
    return Path(os.environ.get("LOCALAPPDATA", "")) / "vcpkg" / "vcpkg.user.props"


def check_vcpkg_integration() -> None:
    if not vcpkg_props_path().is_file():
        raise BuildAbort(
            "[BUILD] FAILED! vcpkg MSBuild integration missing. Run 'vcpkg integrate install' "
            "from the vcpkg that ships with Visual Studio (path is "
            "'<VS install>\\VC\\vcpkg\\vcpkg.exe' inside the VS install -- comes with the C++ "
            "desktop workload).")


def _run(argv: List[str], env: dict = None) -> int:
    return subprocess.run([str(a) for a in argv], env=env, check=False).returncode


class WindowsBuild:
    def __init__(self, root: Path, config: str, mode: str, rebuild: bool,
                 heartbeat: Callable[[], None]) -> None:
        self.root = root
        self.config = config
        self.mode = mode
        self.rebuild = rebuild
        self.heartbeat = heartbeat
        self.toolchain = None

    def discover(self) -> None:
        tc = vs_toolchain.find()
        if not tc.vswhere.is_file():
            raise BuildAbort("[BUILD] FAILED! vswhere.exe not found at {}. Install Visual Studio "
                             "with the C++ desktop workload.".format(tc.vswhere))
        if not tc.tools_version:
            raise BuildAbort(
                "[BUILD] FAILED! No usable MSVC toolset. Installed: {}.\n"
                "[BUILD] Requires >= {} (for /std:c++20) and not 14.51.*, which\n"
                "[BUILD] miscompiles bit-mask dispatch loops on x86 /O2 /Oy- and would silently\n"
                "[BUILD] produce a broken cerf.exe.".format("; ".join(tc.installed),
                                                            vs_toolchain.min_tools_version_text()))
        root = tc.instance["installationPath"]
        if tc.msbuild is None or not tc.msbuild.is_file():
            raise BuildAbort("[BUILD] FAILED! MSBuild.exe not found in {}. Ensure Visual Studio's "
                             "C++ desktop workload is installed.".format(root))
        print("[BUILD] {}, MSVC toolset {} ({}) (installed: {})".format(
            tc.instance.get("displayName", "?"), tc.tools_version, tc.platform_toolset,
            "; ".join(tc.installed)), flush=True)
        self.toolchain = tc

    def _generator(self) -> str:
        cmake = self.toolchain.cmake
        if not cmake.is_file():
            raise BuildAbort("[BUILD] FAILED! CMake not found at {}. Install the 'C++ CMake tools "
                             "for Windows' component of Visual Studio.".format(cmake))
        major = self.toolchain.instance["installationVersion"].split(".")[0]
        caps = subprocess.run([str(cmake), "-E", "capabilities"], capture_output=True, text=True,
                              check=False).stdout
        names = [g.get("name", "") for g in json.loads(caps).get("generators", [])]
        pattern = re.compile(r"^Visual Studio {} \d{{4}}$".format(major))
        match = next((n for n in names if pattern.match(n)), None)
        if match is None:
            raise BuildAbort("[BUILD] FAILED! {} has no generator for Visual Studio {}."
                             .format(cmake, major))
        return match

    def _vcpkg(self) -> Path:
        props = vcpkg_props_path().read_text(encoding="utf-8-sig", errors="replace")
        match = re.search(r'Project="([^"]+)\\scripts\\buildsystems\\msbuild\\vcpkg\.props"', props)
        if match is None:
            raise BuildAbort("[BUILD] FAILED! vcpkg.user.props names no vcpkg root. Run 'vcpkg "
                             "integrate install' again.")
        vcpkg_root = Path(match.group(1))
        installed = self.root / "vcpkg_installed" / VCPKG_TRIPLET
        stamp = installed / ".msbuildstamp-{}.stamp".format(VCPKG_TRIPLET)
        inputs = [self.root / "vcpkg.json", self.root / "vcpkg-configuration.json"]
        current = stamp.is_file() and \
            not any(p.stat().st_mtime > stamp.stat().st_mtime for p in inputs)
        if not current:
            print("[VCPKG] Installing the manifest dependencies ({})".format(VCPKG_TRIPLET),
                  flush=True)
            env = dict(os.environ)
            env["PATH"] = str(self.toolchain.cmake.parent) + os.pathsep + env.get("PATH", "")
            code = _run([vcpkg_root / "vcpkg.exe", "install", "--x-wait-for-lock", "--triplet",
                         VCPKG_TRIPLET, "--vcpkg-root", vcpkg_root,
                         "--x-manifest-root={}".format(self.root),
                         "--x-install-root={}".format(installed)], env)
            self.heartbeat()
            if code != 0:
                raise BuildAbort("[BUILD] vcpkg install FAILED (exit={})".format(code))
            stamp.parent.mkdir(parents=True, exist_ok=True)
            stamp.touch()
        return installed

    def build_libraries(self) -> None:
        generator = self._generator()
        installed = self._vcpkg()
        tc = self.toolchain
        libs_out = self.root / "build" / "cerflibs"
        dev = "OFF" if self.mode == "production" else "ON"
        if self.rebuild and libs_out.exists():
            shutil.rmtree(libs_out)
        wanted = {
            "CMAKE_GENERATOR": generator,
            "CMAKE_GENERATOR_PLATFORM": "Win32",
            "CMAKE_GENERATOR_TOOLSET": "{},version={}".format(tc.platform_toolset,
                                                              tc.tools_version),
            "CMAKE_GENERATOR_INSTANCE": tc.instance["installationPath"],
        }
        cache = libs_out / "CMakeCache.txt"
        if cache.is_file():
            cached = {}
            for line in cache.read_text(encoding="utf-8", errors="replace").splitlines():
                m = re.match(r"^(CMAKE_GENERATOR(?:_PLATFORM|_TOOLSET|_INSTANCE)?):[A-Z]+=(.*)$",
                             line)
                if m:
                    cached[m.group(1)] = m.group(2)
            changed = [k for k in GENERATOR_KEYS if cached.get(k) != wanted[k]]
            if changed:
                print("[LIBS] {} changed; configuring {} from scratch".format(", ".join(changed),
                                                                              libs_out), flush=True)
                shutil.rmtree(libs_out)

        print("[LIBS] libCerfEngine, libCerfCore, libCerfFrontend ({})".format(self.config),
              flush=True)
        code = _run([tc.cmake, "-S", self.root, "-B", libs_out, "-G", generator, "-A", "Win32",
                     "-T", wanted["CMAKE_GENERATOR_TOOLSET"],
                     "-DCMAKE_GENERATOR_INSTANCE={}".format(wanted["CMAKE_GENERATOR_INSTANCE"]),
                     "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>",
                     "-DCMAKE_MSVC_DEBUG_INFORMATION_FORMAT=Embedded",
                     "-DCMAKE_PREFIX_PATH={}".format(installed / VCPKG_TRIPLET),
                     "-DCERFCORE_DEV_MODE=" + dev, "-DCERFCORE_DEVICE_TRACES=" + dev])
        if code == 0:
            code = _run([tc.cmake, "--build", libs_out, "--config", self.config, "--parallel",
                         "--", "/v:minimal", "/nologo", "/nodeReuse:false"])
        self.heartbeat()
        if code != 0:
            raise BuildAbort("[BUILD] CERF libraries FAILED (exit={})".format(code))

    def build_frontend(self) -> bool:
        tc = self.toolchain
        target = "/t:Rebuild" if self.rebuild else "/t:Build"
        if self.rebuild:
            print("[BUILD] Clean rebuild requested (/t:Rebuild)", flush=True)
        dev_flag = "0" if self.mode == "production" else "1"
        code = _run([tc.msbuild, "cerf.sln", "/p:Configuration=" + self.config,
                     "/p:Platform=Win32", target, "/m", "/v:minimal",
                     "/p:CerfExtraDefines=CERF_DEV_MODE=" + dev_flag, "/p:CerfMode=" + self.mode,
                     "/p:VCToolsVersion=" + tc.tools_version])
        self.heartbeat()
        exe = self.root / "build" / self.config / "Win32" / "cerf.exe"
        if code != 0:
            print("[BUILD] cerf.exe FAILED (msbuild exit={})".format(code), flush=True)
            return False
        if not exe.is_file():
            print("[BUILD] cerf.exe FAILED (msbuild reported success but binary not found at {})"
                  .format(exe), flush=True)
            return False
        stat = exe.stat()
        print("[BUILD] cerf.exe OK: {}".format(exe.resolve()), flush=True)
        print("[BUILD] Size: {} bytes".format(stat.st_size), flush=True)
        print("[BUILD] Time: {}".format(datetime.fromtimestamp(stat.st_mtime)), flush=True)
        return True
