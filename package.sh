#!/bin/sh
# Make the installable release candidate from build-amiga.sh's OUT directory.
#
#   package.sh OUT DEST RELEASE COMMIT
#
# RELEASE is the public candidate name (for example 1.4.13-rc1).  The Part
# keeps Aria's upstream version (1.4.13); the release suffix identifies this
# Amiga port candidate without pretending to be a new upstream Aria version.
# GPL-2.0-or-later, like Aria Maestosa.
set -eu

OUT=$1
DEST=$2
RELEASE=$3
COMMIT=$4
BASE=${RELEASE%-rc*}

echo "$RELEASE" | grep -Eq '^[0-9]+(\.[0-9]+)+-rc[1-9][0-9]*$' || {
    echo "release must look like 1.4.13-rc1" >&2
    exit 1
}
echo "$COMMIT" | grep -Eq '^[0-9a-f]{40}$' || {
    echo "commit must be a full 40-character SHA" >&2
    exit 1
}

PART="$OUT/Parts/AriaMaestosa"
BIN="$PART/Utilities/AriaMaestosa/AriaMaestosa"
[ -f "$BIN" ] || { echo "$BIN is missing: run build-amiga.sh first" >&2; exit 1; }
[ -f "$PART/Part" ] || { echo "$PART/Part is missing" >&2; exit 1; }
head -1 "$PART/Part" | grep -qx "part AriaMaestosa $BASE" || {
    echo "Part version does not match $BASE" >&2
    exit 1
}
grep -a -q "\$VER: AriaMaestosa $BASE " "$BIN" || {
    echo "binary version does not match $BASE" >&2
    exit 1
}

mkdir -p "$DEST"
DEST=$(CDPATH= cd -- "$DEST" && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/Parts"
cp -R "$PART" "$TMP/Parts/AriaMaestosa"

BIN_BYTES=$(wc -c < "$BIN" | tr -d ' ')
BIN_SHA=$(sha256sum "$BIN" | awk '{print $1}')
FILE_COUNT=$(find "$PART" -type f | wc -l | tr -d ' ')
cat > "$TMP/PART.json" <<JSON
{"repo":"gfamigaariamaestosa","commit":"$COMMIT","release":"$RELEASE","parts":[{"name":"AriaMaestosa","version":"$BASE","main":"Utilities/AriaMaestosa/AriaMaestosa"}],"binary":{"bytes":$BIN_BYTES,"sha256":"$BIN_SHA"},"files":$FILE_COUNT}
JSON

ARCHIVE="gfamigaariamaestosa-$RELEASE-part.tar.gz"
EPOCH=${SOURCE_DATE_EPOCH:-0}
tar -C "$TMP" --sort=name --mtime="@$EPOCH" --owner=0 --group=0 --numeric-owner \
    -czf "$DEST/$ARCHIVE" PART.json Parts
cp "$TMP/PART.json" "$DEST/PART.json"
(cd "$DEST" && sha256sum "$ARCHIVE" PART.json > SHA256SUMS)

cat "$DEST/SHA256SUMS"
echo "$DEST/$ARCHIVE"
