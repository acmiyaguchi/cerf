import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent.parent / "tools"))
from build_ce_app import run_app

COMMON = dict(type="exe", target="sampleapp.exe", sources=["main.c"], entry="WinMain",
              libs=["coredll", "commctrl", "commdlg"],
              coredll_def=str(HERE.parent / "cerf_guest" / "coredll_byname.def"))

VARIANTS = [
    dict(COMMON, arch="arm", obj_dir="obj_arm"),
    dict(COMMON, arch="mips", obj_dir="obj_mips"),
]

if __name__ == "__main__":
    sys.exit(run_app(HERE, VARIANTS))
