import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent.parent / "tools"))
from build_ce_app import run_app

GUEST = HERE.parent / "cerf_guest"
COMMON = dict(type="exe", target="blankapp.exe", entry="WinMain", libs=["coredll"])

VARIANTS = [
    dict(COMMON, arch="arm", obj_dir="obj_arm", sources=["main.cpp"],
         coredll_def=str(GUEST / "coredll_ce2.def"), wce_version="211", subsystem_version="2.11"),
    dict(COMMON, arch="mips", mips_isa="mips2", obj_dir="obj_mips2",
         sources=["main.cpp", str(GUEST / "cerf_ce2_crt.cpp")],
         coredll_def=str(GUEST / "coredll_ce2.def"), wce_version="211", subsystem_version="2.11"),
    dict(COMMON, arch="mips", mips_isa="mips4", obj_dir="obj_mips4", sources=["main.cpp"],
         coredll_def=str(GUEST / "coredll_byname.def")),
]

if __name__ == "__main__":
    sys.exit(run_app(HERE, VARIANTS))
