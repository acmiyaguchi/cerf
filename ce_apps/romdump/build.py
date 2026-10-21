import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent.parent / "tools"))
from build_ce_app import run_app

GUEST = HERE.parent / "cerf_guest"
SRCS = ["main.cpp", "step_preset.cpp", "step_config.cpp", "step_dump.cpp", "dump.cpp"]
SRCS_CE2 = SRCS + [str(GUEST / "cerf_ce2_crt.cpp")]
SRCS_SH3 = SRCS_CE2 + [str(GUEST / "cerf_rt_sh.cpp")]
CE2_DEF = str(GUEST / "coredll_ce2.def")
COMMON = dict(type="exe", target="romdump.exe", entry="WinMain", libs=["coredll"],
              rc="romdump.rc")

VARIANTS = [
    dict(COMMON, arch="arm", obj_dir="obj_arm", sources=SRCS, coredll_def=CE2_DEF,
         wce_version="100", subsystem_version="2.00"),
    dict(COMMON, arch="mips", mips_isa="mips1", obj_dir="obj_mips1", sources=SRCS_CE2,
         coredll_def=CE2_DEF, wce_version="100", subsystem_version="1.00"),
    dict(COMMON, arch="sh3", obj_dir="obj_sh3", sources=SRCS_SH3, coredll_def=CE2_DEF,
         wce_version="100", subsystem_version="1.00"),
    dict(COMMON, arch="mips", mips_isa="mips4", obj_dir="obj_mips4", sources=SRCS,
         coredll_def=str(GUEST / "coredll_byname.def")),
]

if __name__ == "__main__":
    sys.exit(run_app(HERE, VARIANTS))
