#!/usr/bin/env python3
from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path
from typing import List

ROOT = Path(__file__).resolve().parent.parent

MSI_REL = "SDK/STANDARD_SDK.msi"
TOOL_BIN_REL = "WCE/wce400/BIN"
EVC_BIN_REL = "COMMON/EVC/BIN"
EVC_BIN_FILES = ("rc.exe", "rcdll.dll")
REQUIRED_TOOLS = ("clarm.exe", "clthumb.exe", "clmips.exe", "clsh.exe", "link.exe", "lib.exe",
                  "rc.exe", "rcdll.dll", "cvtres.exe", "armasm.exe", "mipsasm.exe", "mspdb60.dll",
                  "c1_arm.dll", "c1xx_arm.dll", "c2_arm.dll", "c1_mp.dll", "c1xx_mp.dll",
                  "c2_mp.dll", "c1_sh.dll", "c1xx_sh.dll", "c2_sh.dll")
REQUIRED_ARCHES = ("Armv4", "Armv4i", "Mipsii", "Mipsiv", "Sh3")


class Evc4Error(Exception):
    pass


def info(text: str) -> None:
    print("[evc4] " + text, flush=True)


def unpack(evc4_root: Path, out_root: Path, force: bool) -> None:
    if sys.platform != "win32":
        raise Evc4Error("the eVC4 SDK is an MSI package; msiexec extracts it on Windows only")
    if not evc4_root.is_dir():
        raise Evc4Error(
            "eVC4 media not found at:\n    {}\n\nDownload eMbedded Visual C++ 4.0 (English) and "
            "unpack the installer there.\nObtain it from HPC:Factor -- "
            "https://www.hpcfactor.com/developer/\nThen re-run this script. See "
            "docs/ce_apps_setup.md.".format(evc4_root))
    for rel in (MSI_REL, TOOL_BIN_REL, EVC_BIN_REL):
        if not (evc4_root / rel).exists():
            raise Evc4Error(
                "{} does not look like an unpacked eVC4 CD -- missing:\n    {}\n\nExpected layout "
                "(a few landmarks):\n    third_party/evc4/SDK/STANDARD_SDK.msi\n"
                "    third_party/evc4/WCE/wce400/BIN/clarm.exe\n"
                "    third_party/evc4/COMMON/EVC/BIN/rc.exe\n\nUnpack the CD/ISO itself, not a "
                "subfolder of it. See docs/ce_apps_setup.md.".format(evc4_root, rel))
    if out_root.exists() and not force:
        raise Evc4Error("{} already exists. Re-run with --force to rebuild it.".format(out_root))
    if out_root.exists():
        info("removing existing {}".format(out_root))
        shutil.rmtree(str(out_root))
    out_root.mkdir(parents=True)

    msi = (evc4_root / MSI_REL).resolve()
    info("expanding {} (msiexec /a, no installation)".format(MSI_REL))
    code = subprocess.run('msiexec.exe /a "{}" /qn TARGETDIR="{}"'.format(msi, out_root),
                          check=False).returncode
    if code != 0:
        raise Evc4Error("msiexec /a failed with exit code {}".format(code))
    sdk_root = out_root / "STANDARDSDK"
    if not sdk_root.is_dir():
        raise Evc4Error("msiexec succeeded but {} is absent".format(sdk_root))
    for leftover in out_root.glob("*.msi"):
        leftover.unlink()

    bin_out = out_root / "bin"
    bin_out.mkdir(parents=True, exist_ok=True)
    info("copying toolchain from {}".format(TOOL_BIN_REL))
    for item in (evc4_root / TOOL_BIN_REL).iterdir():
        dest = bin_out / item.name
        if item.is_dir():
            shutil.copytree(str(item), str(dest), dirs_exist_ok=True)
        else:
            shutil.copy2(str(item), str(dest))
    info("copying resource compiler from {}".format(EVC_BIN_REL))
    for name in EVC_BIN_FILES:
        src = evc4_root / EVC_BIN_REL / name
        if not src.is_file():
            raise Evc4Error("missing {}/{} in the eVC4 media".format(EVC_BIN_REL, name))
        shutil.copy2(str(src), str(bin_out / name))

    missing: List[str] = ["bin/" + t for t in REQUIRED_TOOLS if not (bin_out / t).exists()]
    for arch in REQUIRED_ARCHES:
        for sub in ("Include", "Lib"):
            if not (sdk_root / sub / arch).exists():
                missing.append("STANDARDSDK/{}/{}".format(sub, arch))
        if not (sdk_root / "Lib" / arch / "coredll.lib").exists():
            missing.append("STANDARDSDK/Lib/{}/coredll.lib".format(arch))
    if missing:
        raise Evc4Error("assembled tree is incomplete -- missing:\n    " + "\n    ".join(missing))

    print("")
    info("OK -- toolchain + SDK assembled at:")
    info("    {}".format(out_root))
    info("  targets: {}".format(", ".join(REQUIRED_ARCHES)))
    print("")
    info("Verify at any time with:  python tools/dev_setup.py --check")


def main(argv: List[str]) -> int:
    parser = argparse.ArgumentParser(description="Assemble the CE toolchain and SDK for ce_apps/.")
    parser.add_argument("--evc4-root", default=str(ROOT / "third_party" / "evc4"))
    parser.add_argument("--out-root", default=str(ROOT / "third_party" / "wince"))
    parser.add_argument("--force", action="store_true")
    args = parser.parse_args(argv)
    try:
        unpack(Path(args.evc4_root), Path(args.out_root), args.force)
    except (Evc4Error, OSError) as exc:
        print("[evc4] ERROR: {}".format(exc), flush=True)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
