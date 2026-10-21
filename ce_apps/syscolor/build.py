import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent.parent / "tools"))
from build_ce_app import run_app

COMMON = dict(type="exe", target="syscolor.exe", entry="WinMain", libs=["coredll"],
              coredll_def=str(HERE.parent / "cerf_guest" / "coredll_ce2.def"), wce_version="100",
              subsystem_version="1.00")

VARIANTS = [
    dict(COMMON, arch="arm", obj_dir="obj_arm", sources=["main.cpp"]),
    dict(COMMON, arch="mips", mips_isa="mips1", obj_dir="obj_mips1",
         sources=["main.cpp", str(HERE.parent / "cerf_guest" / "cerf_ce2_crt.cpp")]),
]

if __name__ == "__main__":
    sys.exit(run_app(HERE, VARIANTS))
