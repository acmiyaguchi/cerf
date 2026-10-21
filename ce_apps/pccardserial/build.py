import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent.parent / "tools"))
from build_ce_app import run_app

CE2_DEF = str(HERE.parent / "cerf_guest" / "coredll_ce2.def")
BYNAME = str(HERE.parent / "cerf_guest" / "coredll_byname.def")
COMMON = dict(type="exe", target="pccardserial.exe", sources=["main.cpp"], entry="WinMain",
              libs=["coredll"])

VARIANTS = [
    dict(COMMON, arch="arm", obj_dir="obj_arm", coredll_def=CE2_DEF, wce_version="100",
         subsystem_version="1.00"),
    dict(COMMON, arch="mips", mips_isa="mips1", obj_dir="obj_mips1", coredll_def=CE2_DEF,
         wce_version="100", subsystem_version="1.00"),
    dict(COMMON, arch="mips", mips_isa="mips4", obj_dir="obj_mips4", coredll_def=BYNAME),
]

if __name__ == "__main__":
    sys.exit(run_app(HERE, VARIANTS))
