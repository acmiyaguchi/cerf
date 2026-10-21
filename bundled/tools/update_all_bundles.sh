#!/bin/sh
here="$(dirname "$0")"
"$here/../launcher.sh" sync update-all
"$here/../launcher.sh" sync download-packages-all
