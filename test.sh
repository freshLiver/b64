#!/bin/bash
TARGET=b64
DATA=data

exit_at() {
    echo "failed at testing size: $1"
    exit 1
}

gcc -o $TARGET test.c b64.c
for SZ in {1..8192}; do
    head -c $SZ /dev/random > $DATA
    ./$TARGET $SZ $DATA > out
    base64 -w 0 < $DATA > ans
    diff out ans || exit_at $SZ
done