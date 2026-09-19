#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

for phase in 1 2 3; do
    echo "===== Phase ${phase}: ctarget ====="
    ./hex2raw < "result${phase}.txt" | ./ctarget -q
done

for phase in 4 5; do
    echo "===== Phase ${phase}: rtarget ====="
    ./hex2raw < "result${phase}.txt" | ./rtarget -q
done
