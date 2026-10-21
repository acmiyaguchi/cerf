#!/usr/bin/env python3
from __future__ import annotations

import os
import sys
import tkinter as tk
import traceback
from pathlib import Path
from typing import List

_THIS_DIR = Path(os.path.abspath(__file__)).parent
if str(_THIS_DIR) not in sys.path:
    sys.path.insert(0, str(_THIS_DIR))
_TOOLS_DIR = _THIS_DIR.parent / "tools"
if _TOOLS_DIR.is_dir() and str(_TOOLS_DIR) not in sys.path:
    sys.path.append(str(_TOOLS_DIR))

from window_icon import apply_window_icon
from installer_window import SETUP_ICO_NAME, InstallerWindow
from text_context_menu import TextContextMenu
from ui_theme import apply_theme, enable_dpi_awareness


def main(argv: List[str]) -> int:
    del argv
    enable_dpi_awareness()

    root = tk.Tk()
    root.withdraw()
    apply_theme(root)
    TextContextMenu(root)
    apply_window_icon(root, SETUP_ICO_NAME)

    try:
        InstallerWindow(root)
        root.mainloop()
    except Exception:
        traceback.print_exc()
        return 1
    finally:
        try:
            root.destroy()
        except tk.TclError:
            pass
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
