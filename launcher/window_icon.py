from __future__ import annotations

import base64
import struct
import sys
import tkinter as tk
from pathlib import Path
from typing import Optional

from app_paths import resolve_icon

_PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def _largest_png_frame(ico: Path) -> Optional[bytes]:
    data = ico.read_bytes()
    if len(data) < 6:
        return None
    _reserved, _kind, count = struct.unpack_from("<HHH", data, 0)
    best = None
    best_size = 0
    for i in range(count):
        entry = 6 + 16 * i
        if entry + 16 > len(data):
            return None
        width, _h, _c, _r, _p, _bpp, size, offset = struct.unpack_from("<BBBBHHII", data, entry)
        frame = data[offset:offset + size]
        if not frame.startswith(_PNG_SIGNATURE):
            continue
        pixels = width or 256
        if pixels > best_size:
            best, best_size = frame, pixels
    return best


def apply_window_icon(window: tk.Misc, name: str = "cerf.ico", default: bool = True) -> None:
    icon = resolve_icon(name)
    if icon is None:
        return
    try:
        if sys.platform == "win32":
            if default:
                window.iconbitmap(default=str(icon))
            else:
                window.iconbitmap(str(icon))
            return
        png = _largest_png_frame(icon)
        if png is None:
            return
        photo = tk.PhotoImage(master=window, data=base64.b64encode(png))
        window.iconphoto(default, photo)
        window._cerf_window_icon = photo
    except tk.TclError:
        pass
