#!/bin/sh
# Host test for the reproducible RC package and its machine-readable receipt.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
OUT="$TMP/out"
PART="$OUT/Parts/AriaMaestosa"
BIN="$PART/Utilities/AriaMaestosa/AriaMaestosa"
mkdir -p "$(dirname "$BIN")"
printf '%s\n' '$VER: AriaMaestosa 1.4.13 (19.11.2026) GFOS 68040' > "$BIN"
printf '%s\n' 'part AriaMaestosa 1.4.13' 'needs 68040' > "$PART/Part"

COMMIT=0123456789abcdef0123456789abcdef01234567
SOURCE_DATE_EPOCH=1 sh "$HERE/../package.sh" "$OUT" "$TMP/a" 1.4.13-rc1 "$COMMIT" >/dev/null
SOURCE_DATE_EPOCH=1 sh "$HERE/../package.sh" "$OUT" "$TMP/b" 1.4.13-rc1 "$COMMIT" >/dev/null

A="$TMP/a/gfamigaariamaestosa-1.4.13-rc1-part.tar.gz"
B="$TMP/b/gfamigaariamaestosa-1.4.13-rc1-part.tar.gz"
cmp "$A" "$B"
(cd "$TMP/a" && sha256sum -c SHA256SUMS)
tar -tzf "$A" | grep -qx 'PART.json'
tar -tzf "$A" | grep -qx 'Parts/AriaMaestosa/Part'
tar -tzf "$A" | grep -qx 'Parts/AriaMaestosa/Utilities/AriaMaestosa/AriaMaestosa'
tar -xOf "$A" PART.json | grep -q '"release":"1.4.13-rc1"'
tar -xOf "$A" PART.json | grep -q "\"commit\":\"$COMMIT\""
tar -xOf "$A" PART.json | grep -q '"version":"1.4.13"'

if sh "$HERE/../package.sh" "$OUT" "$TMP/bad" 1.4.13 "$COMMIT" >/dev/null 2>&1; then
    echo "FAIL: package accepted a non-RC version"
    exit 1
fi
echo "RC package: all checks passed"
