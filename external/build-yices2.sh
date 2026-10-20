#!/bin/sh
# Build the Yices 2 SMT solver from source and install the parts Poling
# needs (headers and the static library) under external/yices2.
#
# Usage: external/build-yices2.sh [VERSION]      (default: 2.7.0)
#
# Requires: curl, a C compiler, make, autoconf, gperf and GMP (with headers).
# Yices is configured without MCSAT, so libpoly and CUDD are not needed.

set -e

VERSION=${1:-2.7.0}
URL="https://github.com/SRI-CSL/yices2/archive/refs/tags/yices-$VERSION.tar.gz"
TOP=$(cd "$(dirname "$0")/.." && pwd)
DEST="$TOP/external/yices2"
JOBS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)

for tool in curl autoconf gperf make; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "build-yices2.sh: '$tool' not found; see INSTALL for the prerequisites." >&2
    exit 1
  fi
done

# Build outside the source tree: Yices' build system does not support
# paths containing spaces.
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

echo "Downloading $URL"
curl -fL "$URL" | tar xz --strip-components 1 -C "$WORK"

cd "$WORK"
autoconf
./configure --prefix="$WORK/install"
make -j"$JOBS"
make install

rm -rf "$DEST"
mkdir -p "$DEST/include" "$DEST/lib"
cp "$WORK"/install/include/*.h "$DEST/include/"
# Only the static library, so that poling does not need LD_LIBRARY_PATH.
cp "$WORK/install/lib/libyices.a" "$DEST/lib/"
cp "$WORK/LICENSE.txt" "$DEST/"

echo "Yices $VERSION installed in $DEST"
