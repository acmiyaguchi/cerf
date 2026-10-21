#!/usr/bin/env python3
from __future__ import annotations

import argparse
import datetime
import os
import re
import sys
from pathlib import Path

VERSION_PATH = Path("libCerfFrontend/include/cerffrontend/version.h")
BUILD_WORD_MODULUS = 65536


def define(text: str, name: str) -> str:
    match = re.search(name + r"\s+(\d+)", text)
    if match is None:
        raise SystemExit("ERROR: {} has no {}".format(VERSION_PATH, name))
    return match.group(1)


def main(argv) -> int:
    parser = argparse.ArgumentParser(description="Stamp version.h with CI build info.")
    parser.add_argument("--run", type=int, required=True)
    parser.add_argument("--sha", required=True)
    args = parser.parse_args(argv)

    text = VERSION_PATH.read_text(encoding="utf-8")
    version = ".".join(define(text, "CERF_VERSION_" + part)
                       for part in ("MAJOR", "MINOR", "PATCH"))
    sha = args.sha[:7]
    stamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%d %H:%M") + " UTC"
    word = args.run % BUILD_WORD_MODULUS

    replacements = (
        (r"^#define\s+CERF_VERSION_BUILD\s+\d+", "#define CERF_VERSION_BUILD      {}".format(args.run)),
        (r"^#define\s+CERF_VERSION_BUILD_WORD\s+\d+", "#define CERF_VERSION_BUILD_WORD {}".format(word)),
        (r"^#define\s+CERF_VERSION_DATE\s+.*$", '#define CERF_VERSION_DATE       "{}"'.format(stamp)),
        (r"^#define\s+CERF_VERSION_SHA\s+.*$", '#define CERF_VERSION_SHA        "{}"'.format(sha)),
    )
    for pattern, line in replacements:
        text = re.sub(pattern, lambda _m, line=line: line, text, flags=re.MULTILINE)
    VERSION_PATH.write_bytes(text.encode("utf-8"))

    output = os.environ.get("GITHUB_OUTPUT")
    if output:
        with open(output, "a", encoding="utf-8") as f:
            f.write("version={}\nsha={}\n".format(version, sha))
    print("Stamped: {} build {} (word {}), {}, {}".format(version, args.run, word, stamp, sha))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
