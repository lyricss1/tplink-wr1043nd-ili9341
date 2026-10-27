#!/bin/sh
set -eu

SDK_DIR="${SDK_DIR:-/opt/openwrt-sdk}"
PROJECT_DIR="${PROJECT_DIR:-/workspace}"
SRC_DIR="$PROJECT_DIR/src"
BUILD_DIR="$PROJECT_DIR/build"

mkdir -p "$BUILD_DIR"

if [ ! -d "$SDK_DIR/staging_dir" ]; then
    echo "Error: OpenWrt SDK not found: $SDK_DIR" >&2
    exit 1
fi

if [ ! -f "$SRC_DIR/badapple2.c" ]; then
    echo "Error: missing $SRC_DIR/badapple2.c" >&2
    exit 1
fi

CC=""
for candidate in "$SDK_DIR"/staging_dir/toolchain-*/bin/*-gcc; do
    if [ -x "$candidate" ]; then
        CC="$candidate"
        break
    fi
done

if [ -z "$CC" ]; then
    echo "Error: MIPS cross-compiler not found in SDK" >&2
    exit 1
fi

echo "SDK: $SDK_DIR"
echo "Compiler: $CC"
"$CC" --version | head -n 1

echo
echo "Building badapple2..."
"$CC" -O2 -Wall -Wextra \
    -o "$BUILD_DIR/badapple2" \
    "$SRC_DIR/badapple2.c"

echo
if [ -f "$SRC_DIR/spibench.c" ]; then
    echo "Building spibench..."
    "$CC" -O2 -Wall -Wextra \
        -o "$BUILD_DIR/spibench" \
        "$SRC_DIR/spibench.c"
else
    echo "Skipping spibench: source file not found"
fi

echo
echo "Build results:"
file "$BUILD_DIR/badapple2"

if [ -f "$BUILD_DIR/spibench" ]; then
    file "$BUILD_DIR/spibench"
fi

echo
echo "Done. Binaries are in: $BUILD_DIR"