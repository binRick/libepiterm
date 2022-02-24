#!/usr/bin/env bash
set -eou pipefail
killall test || true
reset
make clean
make -j
make test && clear && exec ./bin/test 2>.e
