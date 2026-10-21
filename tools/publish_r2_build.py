#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import os
import re
import sys
from pathlib import Path
from typing import List, Set

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import host_branch
from ci_release import CiError, changelog_markdown

VERSION_PATH = Path("libCerfFrontend/include/cerffrontend/version.h")
LATEST_NAME = "latest.json"
LATEST_SCHEMA = 1
BRANCHES_KEY = "host_branches"


def _int_define(text: str, name: str) -> int:
    match = re.search(r"#define\s+" + name + r"\s+(\d+)", text)
    if match is None:
        raise CiError(f"{VERSION_PATH} has no {name}")
    return int(match.group(1))


def _str_define(text: str, name: str) -> str:
    match = re.search(r'#define\s+' + name + r'\s+"([^"]*)"', text)
    if match is None:
        raise CiError(f"{VERSION_PATH} has no {name}")
    return match.group(1)


def version_fields() -> dict:
    if not VERSION_PATH.is_file():
        raise CiError(f"{VERSION_PATH} not found - run from the repo root")
    text = VERSION_PATH.read_text(encoding="utf-8", errors="ignore")
    build = _int_define(text, "CERF_VERSION_BUILD")
    if build == 0:
        raise CiError("CERF_VERSION_BUILD is 0 - version.h was never stamped")
    return {
        "ver_major": _int_define(text, "CERF_VERSION_MAJOR"),
        "ver_minor": _int_define(text, "CERF_VERSION_MINOR"),
        "ver_patch": _int_define(text, "CERF_VERSION_PATCH"),
        "ver_build": build,
        "commit_hash": _str_define(text, "CERF_VERSION_SHA"),
        "build_date": _str_define(text, "CERF_VERSION_DATE"),
    }


def env(name: str) -> str:
    value = os.environ.get(name, "").strip()
    if not value:
        raise CiError(f"{name} is unset or empty")
    return value


def make_client():
    os.environ.setdefault("AWS_REQUEST_CHECKSUM_CALCULATION", "when_required")
    os.environ.setdefault("AWS_RESPONSE_CHECKSUM_VALIDATION", "when_required")
    import boto3
    return boto3.client(
        "s3",
        endpoint_url=f"https://{env('R2_ACCOUNT_ID')}.r2.cloudflarestorage.com",
        aws_access_key_id=env("R2_ACCESS_KEY_ID"),
        aws_secret_access_key=env("R2_SECRET_ACCESS_KEY"),
        region_name="auto",
    )


def stale_zips(client, bucket: str, prefix: str, keep: Set[str]) -> List[str]:
    stale: List[str] = []
    token = None
    while True:
        kwargs = {"Bucket": bucket, "Prefix": prefix + "/"}
        if token:
            kwargs["ContinuationToken"] = token
        page = client.list_objects_v2(**kwargs)
        for item in page.get("Contents", []):
            key = item["Key"]
            if key in keep or not key.lower().endswith(".zip"):
                continue
            if "/" in key[len(prefix) + 1:]:
                continue
            stale.append(key)
        if not page.get("IsTruncated"):
            return stale
        token = page.get("NextContinuationToken")


def read_latest(client, bucket: str, key: str) -> dict:
    try:
        raw = client.get_object(Bucket=bucket, Key=key)["Body"].read()
    except client.exceptions.NoSuchKey:
        return {}
    try:
        parsed = json.loads(raw.decode("utf-8"))
    except ValueError as exc:
        raise CiError(f"{key} is malformed: {exc}") from exc
    if not isinstance(parsed, dict):
        raise CiError(f"{key} is not an object")
    return parsed


def merge_latest(previous: dict, branch: str, entry: dict) -> dict:
    branches = previous.get(BRANCHES_KEY)
    branches = dict(branches) if isinstance(branches, dict) else {}
    branches[branch] = entry
    root = entry if branch == host_branch.WIN32_X86 else \
        {k: v for k, v in previous.items() if k != BRANCHES_KEY}
    latest = dict(root)
    latest["version"] = LATEST_SCHEMA
    latest[BRANCHES_KEY] = branches
    return latest


def referenced_objects(latest: dict, prefix: str) -> Set[str]:
    entries = [latest] + list(latest[BRANCHES_KEY].values())
    return {f"{prefix}/{e['object_name']}" for e in entries
            if isinstance(e, dict) and isinstance(e.get("object_name"), str)}


def argument(argv: List[str], name: str) -> str:
    value = next((a.partition("=")[2] for a in argv if a.startswith(name + "=")), None)
    if not value:
        raise CiError("usage: publish_r2_build.py --zip=<path> --host-branch=<name>")
    return value


def main(argv: List[str]) -> int:
    archive = Path(argument(argv, "--zip"))
    if not archive.is_file():
        raise CiError(f"{archive} not found")
    try:
        branch = host_branch.by_name(argument(argv, "--host-branch")).name
    except host_branch.UnsupportedHost as exc:
        raise CiError(str(exc)) from exc

    bucket = env("R2_BUCKET")
    prefix = env("R2_PREFIX").strip("/")
    if not prefix:
        raise CiError("R2_PREFIX must name a directory, not the bucket root")

    payload = archive.read_bytes()
    entry = {"version": LATEST_SCHEMA}
    entry.update(version_fields())
    entry["object_name"] = archive.name
    entry["object_size"] = len(payload)
    entry["object_sha256"] = hashlib.sha256(payload).hexdigest()

    tag = f"{entry['ver_major']}.{entry['ver_minor']}"
    entry["changelog"] = changelog_markdown(
        tag, f"Nothing recorded in the {tag} changelog yet.")

    client = make_client()
    object_key = f"{prefix}/{archive.name}"
    latest_key = f"{prefix}/{LATEST_NAME}"

    print(f"Uploading {object_key} ({len(payload) / 1024 / 1024:.1f} MB) ...")
    client.put_object(Bucket=bucket, Key=object_key, Body=payload,
                      ContentType="application/zip")

    latest = merge_latest(read_latest(client, bucket, latest_key), branch, entry)
    print(f"Rewriting {latest_key} for {branch} ...")
    client.put_object(
        Bucket=bucket, Key=latest_key,
        Body=json.dumps(latest, indent=2).encode("utf-8"),
        ContentType="application/json", CacheControl="no-cache")

    stale = stale_zips(client, bucket, prefix, referenced_objects(latest, prefix))
    for key in stale:
        print(f"  removing stale {key}")
        client.delete_object(Bucket=bucket, Key=key)

    print(f"Published {branch} build {entry['ver_build']} as {archive.name} "
          f"({len(stale)} stale object(s) removed)")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main(sys.argv[1:]))
    except CiError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(1)
