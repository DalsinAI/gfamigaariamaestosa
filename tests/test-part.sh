#!/bin/sh
# Host test for scripts/write-part.sh: the Part file GFOpenUpTool reads.
# GPL-2.0-or-later. Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
D="$T/Utilities/AriaMaestosa"
mkdir -p "$D/Resources"
: > "$D/AriaMaestosa"; : > "$D/AriaMaestosa.info"; : > "$D/Resources/play.png"; : > "$T/Utilities/AriaMaestosa.info"
sh "$HERE/../scripts/write-part.sh" "$T" 1.4.13
fail=0
expect() { grep -qxF "$1" "$T/Part" || { echo "FAIL: no line: $1"; fail=1; }; }
expect 'part AriaMaestosa 1.4.13'
expect 'needs 68040'
expect 'copy "Utilities/AriaMaestosa/AriaMaestosa" "SYS:Utilities/AriaMaestosa/AriaMaestosa"'
expect 'copy "Utilities/AriaMaestosa/Resources/play.png" "SYS:Utilities/AriaMaestosa/Resources/play.png"'
expect 'default "Utilities/AriaMaestosa/AriaMaestosa.info" "SYS:Utilities/AriaMaestosa/AriaMaestosa.info"'
expect 'default "Utilities/AriaMaestosa.info" "SYS:Utilities/AriaMaestosa.info"'
[ "$(head -1 "$T/Part")" = 'part AriaMaestosa 1.4.13' ] || { echo "FAIL: first line"; fail=1; }
[ "$(grep -c '^copy\|^default' "$T/Part")" = 4 ] || { echo "FAIL: line count (the Part file itself must not be listed)"; fail=1; }
grep -q '^needs part' "$T/Part" && { echo "FAIL: no other part is needed"; fail=1; }
sh "$HERE/../scripts/write-part.sh" "$T" 1.4.13   # a second run: same file
[ "$(grep -c '^copy\|^default' "$T/Part")" = 4 ] || { echo "FAIL: second run lists the Part file"; fail=1; }
[ $fail = 0 ] && echo "Part file: all checks passed"
exit $fail
