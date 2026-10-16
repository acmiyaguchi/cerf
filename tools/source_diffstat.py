from __future__ import annotations

import os
import subprocess
import sys
import tempfile
from typing import Dict, Optional, Set, Tuple

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ci_release import CiError

Pins = Dict[str, str]


def _git(*args: str, env: Optional[dict] = None) -> str:
    result = subprocess.run(["git", *args], capture_output=True, env=env,
                            encoding="utf-8", errors="replace")
    if result.returncode != 0:
        raise CiError(f"git {' '.join(args)}: {result.stderr.strip()}")
    return result.stdout


def _ensure_commit(sha: str) -> None:
    probe = subprocess.run(["git", "cat-file", "-e", f"{sha}^{{commit}}"],
                           capture_output=True)
    if probe.returncode == 0:
        return
    if _git("rev-parse", "--is-shallow-repository").strip() != "true":
        raise CiError(f"commit {sha[:7]} is not in this clone; fetch it first")
    _git("fetch", "--no-tags", "--depth=1", "origin", sha)


def _library_pins(commit: str) -> Pins:
    pins = {}
    for entry in _git("ls-tree", "-z", commit).split("\0"):
        if not entry:
            continue
        meta, _, path = entry.partition("\t")
        _, kind, sha = meta.split()
        if kind == "commit":
            pins[path] = sha
    return pins


def _library_url(commit: str, path: str) -> str:
    blob = f"{commit}:.gitmodules"
    paths = _git("config", "--blob", blob, "--get-regexp",
                 r"^submodule\..*\.path$")
    for line in paths.splitlines():
        key, _, value = line.partition(" ")
        if value == path:
            name = key[:-len(".path")]
            return _git("config", "--blob", blob, "--get", f"{name}.url").strip()
    raise CiError(f"{blob} names no submodule at {path}")


class _CombinedTrees:
    def __init__(self, directory: str) -> None:
        self.directory = directory
        self.libraries = os.path.join(directory, "libraries.git")
        _git("init", "--quiet", "--bare", self.libraries)
        objects = os.path.join(directory, "objects")
        os.makedirs(objects)
        root_objects = os.path.abspath(
            _git("rev-parse", "--git-path", "objects").strip())
        self.env = dict(os.environ, GIT_OBJECT_DIRECTORY=objects,
                        GIT_ALTERNATE_OBJECT_DIRECTORIES=os.pathsep.join(
                            [root_objects,
                             os.path.join(self.libraries, "objects")]))

    def fetch(self, wanted: Dict[str, Set[str]]) -> None:
        env = dict(os.environ, GIT_TERMINAL_PROMPT="0")
        for url, shas in wanted.items():
            _git("-C", self.libraries, "fetch", "--quiet", "--no-tags",
                 "--depth=1", url, *sorted(shas), env=env)

    def tree(self, label: str, commit: str, expand: Pins) -> str:
        env = dict(self.env,
                   GIT_INDEX_FILE=os.path.join(self.directory, f"index-{label}"))
        _git("read-tree", commit, env=env)
        for path, sha in expand.items():
            _git("update-index", "--force-remove", "--", path, env=env)
            _git("read-tree", f"--prefix={path}/", f"{sha}^{{tree}}", env=env)
        return _git("write-tree", env=env).strip()

    def numstat(self, base_tree: str, head_tree: str) -> Tuple[int, int]:
        added = deleted = 0
        output = _git("diff", "--numstat", "--find-renames", "-l0",
                      base_tree, head_tree, env=self.env)
        for line in output.splitlines():
            plus, minus, _ = line.split("\t", 2)
            if plus != "-":
                added += int(plus)
                deleted += int(minus)
        return added, deleted


def diffstat(base: str, head: str) -> Tuple[int, int]:
    for sha in (base, head):
        _ensure_commit(sha)
    base_pins, head_pins = _library_pins(base), _library_pins(head)
    changed = {path for path in set(base_pins) | set(head_pins)
               if base_pins.get(path) != head_pins.get(path)}
    base_expand = {path: base_pins[path] for path in changed if path in base_pins}
    head_expand = {path: head_pins[path] for path in changed if path in head_pins}
    wanted: Dict[str, Set[str]] = {}
    for commit, expand in ((base, base_expand), (head, head_expand)):
        for path, sha in expand.items():
            wanted.setdefault(_library_url(commit, path), set()).add(sha)
    with tempfile.TemporaryDirectory(prefix="cerf_diffstat_") as directory:
        trees = _CombinedTrees(directory)
        trees.fetch(wanted)
        return trees.numstat(trees.tree("base", base, base_expand),
                             trees.tree("head", head, head_expand))
