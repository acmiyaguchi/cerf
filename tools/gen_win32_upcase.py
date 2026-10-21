#!/usr/bin/env python3
import bisect
import ctypes
import sys
from pathlib import Path

OUTPUT = Path(__file__).resolve().parent.parent / "libCerfCore" / "src" / "platform" / \
    "host_win32_upcase_table.h"


def windows_upcase_table():
    upcase = ctypes.WinDLL("ntdll").RtlUpcaseUnicodeChar
    upcase.restype = ctypes.c_uint16
    upcase.argtypes = [ctypes.c_uint16]
    return [upcase(c) for c in range(0x10000)]


def runs_of(table):
    pairs = [(c, table[c]) for c in range(0x10000) if table[c] != c]
    runs = []
    i = 0
    while i < len(pairs):
        first, upper = pairs[i]
        delta = upper - first
        best_len, best_step = 1, 1
        for step in (1, 2):
            n = 1
            while (i + n < len(pairs) and pairs[i + n][0] == first + n * step and
                   pairs[i + n][1] - pairs[i + n][0] == delta):
                n += 1
            if n > best_len:
                best_len, best_step = n, step
        runs.append((first, first + (best_len - 1) * best_step, best_step, delta))
        i += best_len
    return runs


def check_runs(runs, table):
    for prev, run in zip(runs, runs[1:]):
        if run[0] <= prev[1]:
            raise SystemExit("runs {:04X}-{:04X} and {:04X}-{:04X} overlap".format(
                prev[0], prev[1], run[0], run[1]))
    lasts = [run[1] for run in runs]
    for c in range(0x10000):
        i = bisect.bisect_left(lasts, c)
        upper = c
        if i < len(runs):
            first, _, step, delta = runs[i]
            if c >= first and (c - first) % step == 0:
                upper = c + delta
        if upper != table[c]:
            raise SystemExit("lookup gives {:04X} for {:04X}, ntdll gives {:04X}".format(
                upper, c, table[c]))


def header(runs):
    lines = [
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "namespace cerfcore::host_platform {",
        "",
        "struct Win32UpcaseRun {",
        "    char16_t first;",
        "    char16_t last;",
        "    uint16_t step;",
        "    int32_t  delta;",
        "};",
        "",
        "inline constexpr Win32UpcaseRun kWin32UpcaseRuns[] = {",
    ]
    for first, last, step, delta in runs:
        lines.append("    {{0x{:04X}, 0x{:04X}, {}, {}}},".format(first, last, step, delta))
    lines += ["};", "", "}", ""]
    return "\n".join(lines)


def main():
    if sys.platform != "win32":
        raise SystemExit("gen_win32_upcase.py reads the table from ntdll and runs on Windows only")
    table = windows_upcase_table()
    runs = runs_of(table)
    check_runs(runs, table)
    OUTPUT.write_text(header(runs), encoding="utf-8", newline="\n")
    print("wrote", OUTPUT)


if __name__ == "__main__":
    main()
