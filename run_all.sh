#!/bin/sh
# Build sum_int.c and sum_float.c at every optimization level and run each.
set -e
CC=${CC:-gcc}

for prog in sum_int sum_float; do
    echo "== $prog ($CC $EXTRA) =="
    for opt in O0 O1 O2 O3 Ofast; do
        $CC -std=c11 -Wall -$opt $EXTRA -o ${prog}_$opt $prog.c
        printf -- '-%-6s ' "$opt"
        ./${prog}_$opt
    done
    echo
done