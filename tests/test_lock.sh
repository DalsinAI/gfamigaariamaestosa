#!/bin/sh
# Host test: aria.lock pins full commits, the version agrees with the $VER
# in amiga/amiga_stack.c, and every patch applies to Aria at its commit.
# GPL-2.0-or-later. Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$HERE/../aria.lock"
fail=0
for c in "$ARIA_COMMIT" "$WXT_COMMIT" "$FPA_COMMIT"; do
    echo "$c" | grep -qE '^[0-9a-f]{40}$' || { echo "FAIL: not a full commit: $c"; fail=1; }
done
grep -q "\$VER: AriaMaestosa $ARIA_VERSION (" "$HERE/../amiga/amiga_stack.c" || { echo "FAIL: \$VER is not $ARIA_VERSION"; fail=1; }
grep -q 'os32-gcc16-open8' "$HERE/../build-amiga.sh" || { echo "FAIL: open8 is not the default stove"; fail=1; }
grep -q 'TOOLCHAIN_ID=' "$HERE/../build-amiga.sh" || { echo "FAIL: build cache does not identify the toolchain"; fail=1; }
grep -q 'OBJREV=.*TOOLCHAIN_ID' "$HERE/../build-amiga.sh" || { echo "FAIL: Aria object cache ignores the toolchain"; fail=1; }
grep -q 'WXREV=.*TOOLCHAIN_ID' "$HERE/../build-amiga.sh" || { echo "FAIL: toolkit cache ignores the toolchain"; fail=1; }
if [ "${ARIA_SRC:-}" ]; then
    T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
    cp -R "$ARIA_SRC/." "$T/"
    [ "$(git -C "$T" rev-parse HEAD)" = "$ARIA_COMMIT" ] || { echo "FAIL: ARIA_SRC is not at $ARIA_COMMIT"; fail=1; }
    for p in "$HERE"/../patches/*.patch; do patch -s -d "$T" -p1 < "$p" || { echo "FAIL: $p"; fail=1; }; done
    grep -q 'PROGDIR:Resources/' "$T/Src/IO/IOUtils.cpp" || { echo "FAIL: resources path"; fail=1; }
    grep -q '#if defined(__amigaos__)' "$T/rtmidi/RtMidi.cpp" || { echo "FAIL: rtmidi section"; fail=1; }
    # the Amiga RtMidi section compiles (host, as the Amiga build has no other backend)
    ${CXX:-c++} -std=gnu++17 -D__amigaos__ -fsyntax-only -I"$T/rtmidi" "$T/rtmidi/RtMidi.cpp" || { echo "FAIL: RtMidi.cpp"; fail=1; }
fi
[ $fail = 0 ] && echo "lock and patches: all checks passed"
exit $fail
