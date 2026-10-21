from __future__ import annotations

import re
from functools import lru_cache
from typing import Dict, List, Tuple

from app_paths import EVDEV_VK_HEADER, HOST_KEY_NAMES_HEADER, resolve_source_header

_HEX = re.compile(r"0x[0-9A-Fa-f]+")
_SIDED = re.compile(r"\{\s*(\d+)\s*,\s*(0x[0-9A-Fa-f]+)\s*\}")
_NAME = re.compile(r'\{\s*(0x[0-9A-Fa-f]+)\s*,\s*"((?:[^"\\]|\\.)*)"\s*\}')


def _header_text(header) -> str:
    path = resolve_source_header(header)
    if path is None:
        raise FileNotFoundError("{} is missing from this installation".format(header[0]))
    return path.read_text(encoding="utf-8")


def _block(text: str, symbol: str) -> str:
    start = text.index(symbol)
    open_brace = text.index("{", text.index("=", start))
    return text[open_brace + 1:text.index("\n};", open_brace)]


@lru_cache(maxsize=None)
def _position_tables() -> Tuple[int, List[int], Dict[int, int]]:
    text = _header_text(EVDEV_VK_HEADER)
    offset = int(re.search(r"kXKeycodeEvdevOffset\s*=\s*(\d+)", text).group(1))
    evdev_to_vk = [int(v, 16) for v in _HEX.findall(_block(text, "kEvdevToVk["))]
    sided = {int(code): int(vk, 16)
             for code, vk in _SIDED.findall(_block(text, "kEvdevSidedVks[]"))}
    return offset, evdev_to_vk, sided


@lru_cache(maxsize=None)
def key_names() -> Dict[int, str]:
    text = _header_text(HOST_KEY_NAMES_HEADER)
    return {int(vk, 16): re.sub(r"\\(.)", r"\1", name)
            for vk, name in _NAME.findall(_block(text, "kHostKeyNames[]"))}


def vk_from_x_keycode(keycode: int) -> int:
    offset, evdev_to_vk, sided = _position_tables()
    evdev = keycode - offset
    if evdev < 0 or evdev >= len(evdev_to_vk):
        return 0
    vk = evdev_to_vk[evdev]
    return sided.get(evdev, vk) if vk else 0
