#!/bin/sh

ARCHIVE="/overlay/badapple.baf.gz"
VIDEO="/tmp/badapple.baf"
PLAYER="/root/badapple"
FPS="${1:-30}"

if [ ! -x "$PLAYER" ]; then
    echo "$PLAYER not found, or need chmod+X"
    exit 1
fi

if [ ! -f "$ARCHIVE" ]; then
    echo "No archive $ARCHIVE err"
    exit 1
fi

echo "Bad Apple in RAM..."
if ! gzip -dc "$ARCHIVE" > "$VIDEO"; then
    echo "err archive"
    rm -f "$VIDEO"
    exit 1
fi

echo "Bad apple $FPS"
"$PLAYER" "$VIDEO" "$FPS"
RESULT=$?

rm -f "$VIDEO"
echo ""
exit "$RESULT"
