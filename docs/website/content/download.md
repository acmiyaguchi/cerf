---
hide:
  - navigation
---

# Download

[:fontawesome-solid-download: Download Installer (Windows, x86)](https://cerf-bundles.dz3n.net/cerf_installer/cerf_installer.exe){ .md-button .md-button--primary }

<br>

!!! warning "Supported platforms"

    **Recommended**: Latest Windows version
    
    **Minimal requirements:** Windows Vista. Windows XP is partially supported without UI components. This is best effort support, might break or get removed at any time.

## Linux

Linux has no installer. Download the `Release-linux-qt-amd64` zip from the [latest release](https://github.com/gweslab/cerf/releases/latest). Extract it, then run `launcher.sh`.

The build is for x86-64, and it uses the Qt 6.10 libraries of your system. On Ubuntu 26.04, install these packages first:

```
sudo apt install libqt6widgets6 libqt6dbus6 libqt6svg6 qt6-qpa-plugins libslirp0 libasound2t64
```

## Unstable builds

If you would like to use unstable builds, built directly from the tree which are pending release - you can:

- open the launcher, Settings dialog and pick the unstable release channel. You will upgrade to one instantly.
- or you can download builds directly at [build list](https://github.com/gweslab/cerf/actions/workflows/build.yml?query=branch%3Amain+is%3Asuccess)
