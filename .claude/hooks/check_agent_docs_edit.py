#!/usr/bin/env python3
import json
import os
import sys

import _hookpath


def main() -> int:
    try:
        payload = json.loads(sys.stdin.buffer.read().decode("utf-8-sig"))
    except (json.JSONDecodeError, ValueError, UnicodeDecodeError):
        return 0

    tool_input = payload.get("tool_input") or {}
    tool_response = payload.get("tool_response") or {}
    file_path = _hookpath.normalize(tool_response.get("filePath") or tool_input.get("file_path"))
    if not file_path:
        return 0

    try:
        rel = os.path.relpath(file_path).replace("\\", "/")
    except ValueError:
        rel = file_path.replace("\\", "/")

    if rel != "CLAUDE.md" and not rel.startswith("agent_docs/"):
        return 0

    msg = (
        f"You edited {rel}. If the task needs this edit, keep it and run "
        f"/leak and /simple-english. Otherwise revert it."
    )

    json.dump({
        "hookSpecificOutput": {
            "hookEventName": "PostToolUse",
            "additionalContext": msg,
        },
        "systemMessage": f"[CLAUDE.md hook] project docs edited: {rel}",
    }, sys.stdout)
    return 0


if __name__ == "__main__":
    sys.exit(main())
