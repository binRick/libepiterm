#!/usr/bin/env bash
set -eou pipefail
killall test || true
reset
[[ -f bin/epi ]] && unlink bin/epi
make clean && make -j && make epi -j && clear && exec ./bin/epi 2>.e
