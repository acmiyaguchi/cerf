from __future__ import annotations

from typing import Optional

import host_branch
from app_settings import (CHANNEL_DISABLED, CHANNEL_UNSTABLE,
                          read_update_channel)
from available_update import AvailableUpdate
from bundle_repositories import config_path, load_config
from bundles import BundleError, DEFAULT_TIMEOUT
from update_channel_stable import fetch_latest_release
from update_channel_unstable import fetch_latest_ci_build


def installed_branch() -> host_branch.HostBranch:
    try:
        return host_branch.from_config(load_config(config_path()))
    except host_branch.UnsupportedHost as exc:
        raise BundleError(str(exc)) from exc


def fetch_update(timeout: int = DEFAULT_TIMEOUT) -> Optional[AvailableUpdate]:
    channel = read_update_channel()
    if channel == CHANNEL_DISABLED:
        return None
    branch = installed_branch()
    if channel == CHANNEL_UNSTABLE:
        return fetch_latest_ci_build(branch, timeout)
    return fetch_latest_release(branch, timeout)
