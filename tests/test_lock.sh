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
grep -q 'objects.txt' "$HERE/../build-amiga.sh" || { echo "FAIL: linker has no current-object manifest"; fail=1; }
grep -q '@amiga/CamdMidiManager.cpp' "$HERE/../build-amiga.sh" || { echo "FAIL: project object name depends on checkout path"; fail=1; }
grep -q 'cluster\[64\] = "akikoplus.out.0"' "$HERE/../amiga/camd_player.c" || { echo "FAIL: AkikoPlus/OpenSynth is not the default MIDI route"; fail=1; }
if [ "${ARIA_SRC:-}" ]; then
    T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
    cp -R "$ARIA_SRC/." "$T/"
    [ "$(git -C "$T" rev-parse HEAD)" = "$ARIA_COMMIT" ] || { echo "FAIL: ARIA_SRC is not at $ARIA_COMMIT"; fail=1; }
    for p in "$HERE"/../patches/*.patch; do patch -s -d "$T" -p1 < "$p" || { echo "FAIL: $p"; fail=1; }; done
    grep -q 'PROGDIR:Resources/' "$T/Src/IO/IOUtils.cpp" || { echo "FAIL: resources path"; fail=1; }
    grep -q '#if defined(__amigaos__)' "$T/rtmidi/RtMidi.cpp" || { echo "FAIL: rtmidi section"; fail=1; }
    grep -q 'defined(__WXMSW__) || defined(__amigaos__)' "$T/Src/GUI/MainFrame.cpp" || { echo "FAIL: GFOpenWX first-show initialisation"; fail=1; }
    grep -q 'preferredMidiDriver = wxT("CAMD")' "$T/Src/Midi/Players/PlatformMidiManager.cpp" || { echo "FAIL: Amiga does not select the CAMD backend"; fail=1; }
    grep -q 'fitSingleTrackToWindow' "$T/Src/GUI/MainFrame.cpp" || { echo "FAIL: Amiga one-track editor does not fill the window"; fail=1; }
    grep -q 'wxSYS_COLOUR_WINDOW' "$T/Src/Renderers/wxRenderPane.cpp" || { echo "FAIL: Amiga editor surface does not follow the shared theme"; fail=1; }
    grep -q 'defined(__WXGTK__) || defined(__amigaos__)' "$T/Src/GUI/MainFrame.h" || { echo "FAIL: Amiga toolbar bypasses OpenLayout controls"; fail=1; }
    grep -q 'wxBitmapToggleButton' "$T/Src/GUI/MainFrame.cpp" || { echo "FAIL: Amiga loop control is not an OpenLayout toggle"; fail=1; }
    grep -q 'defined(__WXMAC__) || defined(__amigaos__)' "$T/Src/GUI/MainFrame.h" || { echo "FAIL: Amiga numeric fields use unsupported composite spin controls"; fail=1; }
    # the Amiga RtMidi section compiles (host, as the Amiga build has no other backend)
    ${CXX:-c++} -std=gnu++17 -D__amigaos__ -fsyntax-only -I"$T/rtmidi" "$T/rtmidi/RtMidi.cpp" || { echo "FAIL: RtMidi.cpp"; fail=1; }
fi
[ $fail = 0 ] && echo "lock and patches: all checks passed"
exit $fail
