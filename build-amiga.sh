#!/bin/sh
# Builds Aria Maestosa for GFOS (AmigaOS 3.2.3 with GFOpenUp) with a stove:
# 68040, soft float with the FPU taken when there is one (fpuauto), on the
# GFOpenWX Toolkit, with wx's own drawing (no OpenGL, no GFOpenGPU) and a
# CAMD player of our own (amiga/).
#   STOVE=.../stoves/os32-gcc16-open6 OUT=out ./build-amiga.sh
#   (WXT_SRC=... to use a toolkit tree already on disk)
# Aria, the toolkit and fpuauto are fetched at the commits aria.lock gives;
# nothing third-party is kept here. The build tree is kept, so a run that
# stops at BUDGET_TOTAL seconds goes on next time.
# OUT gets Parts/AriaMaestosa: a GFOpenUp part (its Part file and the files
# it puts in SYS:Utilities/AriaMaestosa), installed with
#   GFOpenUpTool INSTALL Parts/AriaMaestosa
# GPL-2.0-or-later, like Aria Maestosa. Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$HERE/aria.lock"
STOVE=${STOVE:-"$HOME/AmiCradle/stoves/os32-gcc16-open6"}
# outside the checkout: Cradle cleans it (git clean -ffdx) before every job
WORK="${WORK:-$HOME/.cache/gfamigaariamaestosa/build}"; OUT="${OUT:-$WORK/out}"; JOBS=${JOBS:-12}
TC="$STOVE/prefix"; PATH="$TC/bin:$PATH"; export PATH
mkdir -p "$WORK/deps" "$OUT"; OUT=$(cd "$OUT" && pwd)
t0=$(date +%s)
m68k-amigaos-gcc --version | head -n 1

# a repository at one commit, fetched once (SPARSE: only those folders)
fetch() {
    d="$WORK/deps/$1"
    if [ "$(git -C "$d" rev-parse HEAD 2>/dev/null)" != "$3" ]; then
        rm -rf "${d:?}"; mkdir -p "$d"
        git -C "$d" init -q
        if [ -n "${4:-}" ]; then
            git -C "$d" config core.sparseCheckout true
            echo "$4" > "$d/.git/info/sparse-checkout"
            git -C "$d" fetch -q --depth 1 --filter=blob:none "$2" "$3"
        else
            git -C "$d" fetch -q --depth 1 "$2" "$3"
        fi
        git -C "$d" checkout -q FETCH_HEAD
        [ "$(git -C "$d" rev-parse HEAD)" = "$3" ] || { echo "$1: commit mismatch"; exit 1; }
    fi
}

# --- the toolkit's SDK, again only when its tree changed ---------------------
if [ -z "${WXT_SRC:-}" ]; then fetch gfopenwxtoolkit "$WXT_REPO" "$WXT_COMMIT"; WXT_SRC="$WORK/deps/gfopenwxtoolkit"; fi
WXB="$WORK/gfopenwx"
WXREV=$( (git -C "$WXT_SRC" rev-parse HEAD 2>/dev/null; git -C "$WXT_SRC" diff HEAD 2>/dev/null) | sha256sum | cut -c1-16)
if [ ! -x "$WXB/out/sdk/bin/wx-config" ] || [ "$(cat "$WXB/out/.rev" 2>/dev/null)" != "$WXREV" ]; then
    # stopped here, before Cradle's own cap, so the log says how far it got
    set +e
    STOVE="$STOVE" WORK="$WXB" OUT="$WXB/out" JOBS=$JOBS timeout "${BUDGET_TOTAL:-840}" sh "$WXT_SRC/build-amiga.sh" > "$WORK.wx.log" 2>&1
    rc=$?
    set -e
    if [ "$rc" = 124 ]; then
        echo "toolkit: out of time after $(( $(date +%s) - t0 )) s, run again to go on"
        echo "  base objects: $(find "$WXB/base" -name '*.o' 2>/dev/null | wc -l), GUI objects: $(find "$WXB/gui/obj" -name '*.o' 2>/dev/null | wc -l)"
        tail -5 "$WORK.wx.log"
        exit 0
    fi
    [ "$rc" = 0 ] || { tail -40 "$WORK.wx.log"; exit 1; }
    [ -x "$WXB/out/sdk/bin/wx-config" ] || { tail -40 "$WORK.wx.log"; echo "no toolkit SDK"; exit 1; }
    echo "$WXREV" > "$WXB/out/.rev"
fi
echo "toolkit: $(( $(date +%s) - t0 )) s"
WXC="$WXB/out/sdk/bin/wx-config"

# --- fpuauto: libfpuauto.a and its link specs ---------------------------------
fetch fpuauto "$FPA_REPO" "$FPA_COMMIT" "/common/fpuauto/"
FA="$WORK/deps/fpuauto/common/fpuauto"; FAL="$WORK/fpuauto"
if [ ! -f "$FAL/libfpuauto.a" ] || [ "$(cat "$FAL/.rev" 2>/dev/null)" != "$FPA_COMMIT" ]; then
    rm -rf "$FAL"; mkdir -p "$FAL/obj"
    m68k-amigaos-gcc -m68040 -DFPUAUTO_COMMON -c "$FA/fpuauto.S" -o "$FAL/obj/fa_common.o"
    for n in $(sed 's/^--wrap=//' "$FA/fpuauto.wraps"); do
        m68k-amigaos-gcc -m68040 -DR_$n -c "$FA/fpuauto.S" -o "$FAL/obj/fa_$n.o"
    done
    m68k-amigaos-ar rcs "$FAL/libfpuauto.a" "$FAL"/obj/fa_*.o
    echo "$FPA_COMMIT" > "$FAL/.rev"
fi

# --- Aria at its pinned commit, with patches/ ---------------------------------
SRC="$WORK/aria"
PSUM=$(cat "$HERE"/patches/*.patch | sha256sum | cut -c1-16)
if [ "$(cat "$SRC/.patched" 2>/dev/null)" != "$ARIA_COMMIT-$PSUM" ]; then
    rm -rf "$SRC"; mkdir -p "$SRC"
    git -C "$SRC" init -q
    git -C "$SRC" fetch -q --depth 1 "$ARIA_GIT" "$ARIA_COMMIT"
    git -C "$SRC" checkout -q FETCH_HEAD
    [ "$(git -C "$SRC" rev-parse HEAD)" = "$ARIA_COMMIT" ] || { echo "commit mismatch"; exit 1; }
    for p in "$HERE"/patches/*.patch; do
        patch -s -d "$SRC" -p1 < "$p" || { echo "patch failed: $p"; exit 1; }
    done
    echo "$ARIA_COMMIT-$PSUM" > "$SRC/.patched"
    rm -rf "$WORK/obj"
fi
# objects made against another toolkit commit go: its headers are not in
# the rules, and stale objects crash at start (gfamigagolly #3)
if [ "$(cat "$WORK/.toolkit-objs" 2>/dev/null)" != "$WXT_COMMIT" ]; then
    rm -rf "$WORK/obj"
    echo "$WXT_COMMIT" > "$WORK/.toolkit-objs"
fi

# --- compile: every .cpp of Aria's tree (as its SConstruct), ours from amiga/ -
LATE=""
if m68k-amigaos-gcc -fno-late-combine-instructions -S -o /dev/null -x c /dev/null 2>/dev/null; then LATE="-fno-late-combine-instructions"; fi
CF="-noixemul -m68040 -O2 -fno-delete-null-pointer-checks $LATE -DNDEBUG=1"
O="$WORK/obj"; mkdir -p "$O"
CXXF="$CF -std=gnu++17 $("$WXC" --cxxflags) -DRENDERER_WXWIDGETS -I$SRC/Src -I$SRC -I$SRC/libjdkmidi/include -I$SRC/rtmidi -I$HERE/amiga -Wno-deprecated-declarations -Wno-register"
export CXXF CF O SRC HERE
(cd "$SRC" && find . -name '*.cpp' ! -path './scons/*' ! -path './.git/*' | LC_ALL=C sort) > "$WORK/sources.txt"
echo "$HERE/amiga/CamdMidiManager.cpp" >> "$WORK/sources.txt"
# one object per source, named by its path; kept when newer than the source
left=$(( ${BUDGET_TOTAL:-840} - ($(date +%s) - t0) ))
[ "$left" -gt 60 ] || { echo "no time left for Aria this run"; exit 0; }
set +e
timeout "$left" xargs -P "$JOBS" -I{} sh -c '
    s={}; case "$s" in /*) f=$s ;; *) f=$SRC/${s#./} ;; esac
    o=$O/$(echo "${s#./}" | tr "/" "_").o
    [ -f "$o" ] && [ "$o" -nt "$f" ] && exit 0
    nice -n 19 m68k-amigaos-g++ $CXXF -c "$f" -o "$o.tmp" > "$o.log" 2>&1 && mv "$o.tmp" "$o" && rm -f "$o.log"
' < "$WORK/sources.txt"
rc=$?
set -e
nsrc=$(wc -l < "$WORK/sources.txt"); nobj=$(find "$O" -name '*.o' | wc -l)
echo "compile: $nobj of $nsrc objects, $(( $(date +%s) - t0 )) s"
fails=$(find "$O" -name '*.o.log' | LC_ALL=C sort)
if [ -n "$fails" ]; then
    for l in $fails; do echo "== $(basename "$l" .o.log)"; grep -E 'error' "$l" | head -8; done | head -120
    exit 1
fi
[ "$rc" = 0 ] || { [ "$rc" = 124 ] && echo "out of time: run again to go on"; exit 0; }
[ "$nobj" -ge "$nsrc" ] || { echo "objects missing"; exit 1; }
for c in camd_smf camd_player amiga_stack; do
    m68k-amigaos-gcc $CF -std=gnu99 -I"$HERE/amiga" -c "$HERE/amiga/$c.c" -o "$O/amiga_c_$c.o"
done

# --- the link: soft-float specs, then fpuauto's; our player's objects first ---
# shellcheck disable=SC2046
m68k-amigaos-g++ -noixemul -m68040 -specs="$HERE/scripts/m68k-soft-m68040.specs" -specs="$FA/fpuauto.specs" -L"$FAL" \
    -o "$O/AriaMaestosa.debug" "$O"/*CamdMidiManager.cpp.o $(ls "$O"/*.o | grep -v CamdMidiManager) \
    $("$WXC" --libs std,aui,xml,net) > "$O/link.log" 2>&1 || { head -80 "$O/link.log"; exit 1; }

# --- the GFOpenUp part: Parts/AriaMaestosa ------------------------------------
P="$OUT/Parts/AriaMaestosa"; D="$P/Utilities/AriaMaestosa"; rm -rf "$P"; mkdir -p "$D"
m68k-amigaos-strip -o "$D/AriaMaestosa" "$O/AriaMaestosa.debug"
cp -R "$SRC/Resources" "$D/Resources"
cp "$SRC/license.txt" "$D/License.txt"
cp "$HERE/README.md" "$D/ReadMe"
cp "$HERE/amiga/icons/AriaMaestosa.info" "$D/AriaMaestosa.info"
cp "$HERE/amiga/icons/ReadMe.info" "$D/ReadMe.info"
cp "$HERE/amiga/icons/ReadMe.info" "$D/License.txt.info"
mkdir -p "$P/Utilities"; cp "$HERE/amiga/icons/AriaMaestosa-drawer.info" "$P/Utilities/AriaMaestosa.info"
sh "$HERE/scripts/write-part.sh" "$P" "$ARIA_VERSION"
ls -l "$D/AriaMaestosa"
echo "part: $(grep -c '^copy' "$P/Part") files, $(du -sk "$P" | cut -f1) KB"
echo "fpuauto: $(m68k-amigaos-nm "$O/AriaMaestosa.debug" | grep -c ' T ___wrap_') wrapped routines"
echo "total: $(( $(date +%s) - t0 )) s"
