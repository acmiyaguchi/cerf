import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent.parent / "tools"))
from build_ce_app import run_app

GUEST = HERE.parent / "cerf_guest"
SRC = ["main.c", "xplorer_view.c", "xplorer_icons.c", "xplorer_desktop.c", "xplorer_taskbar.c",
       "xplorer_taskmgr.c", "xplorer_run.c", "xplorer_navglyph.c"]
ALL_SRC = SRC + [str(GUEST / "cerf_getversionexw.cpp")]
CE2_SRC = ALL_SRC + [str(GUEST / "cerf_ce2_crt.cpp")]
CE2_DEF = str(GUEST / "coredll_ce2.def")
COMMON = dict(type="exe", target="xplorer.exe", entry="WinMain", libs=["coredll"])

VARIANTS = [
    dict(COMMON, arch="arm", obj_dir="obj_arm", sources=CE2_SRC, coredll_def=CE2_DEF,
         wce_version="211", subsystem_version="2.11"),
    dict(COMMON, arch="mips", mips_isa="mips2", obj_dir="obj_mips2", sources=CE2_SRC,
         coredll_def=CE2_DEF, wce_version="211", subsystem_version="2.11"),
    dict(COMMON, arch="mips", mips_isa="mips4", obj_dir="obj_mips4", sources=ALL_SRC,
         coredll_def=str(GUEST / "coredll_byname.def")),
]

if __name__ == "__main__":
    sys.exit(run_app(HERE, VARIANTS))
