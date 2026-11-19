#!/bin/sh
# Writes the Part file GFOpenUpTool INSTALL reads (gfopenup src/openuptool.c):
# "copy" for each file under the part drawer, each to SYS:<its path>; the
# drawer and program icons as "default", so the user's icon places stay.
#   write-part.sh <part drawer> <version>
# GPL-2.0-or-later. Copyright (c) 2026 AmiCradle Team.
set -eu
P=$1; V=$2
{
    echo "part AriaMaestosa $V"
    echo "; Aria Maestosa: write music on score, keyboard, guitar and drum views; plays through CAMD, saves MIDI"
    echo "needs 68040"
    (cd "$P" && find Utilities -type f ! -name Part | LC_ALL=C sort) | while read -r f; do
        case "$f" in
            Utilities/AriaMaestosa.info|Utilities/AriaMaestosa/AriaMaestosa.info) echo "default \"$f\" \"SYS:$f\"" ;;
            *) echo "copy \"$f\" \"SYS:$f\"" ;;
        esac
    done
} > "$P/Part"
