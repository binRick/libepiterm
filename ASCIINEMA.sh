#!/bin/sh
set -eou pipefail
export ASCIINEMA_API_URL=http://localhost:12316 SESS=t100 
set -- ~/.palettes/*dark*

vtreset(){
  vterm-ctrl reset
  vterm-ctrl cursor off
  vterm-ctrl curshape bar
  vterm-ctrl curblink on
  vterm-ctrl title xxxxxxxxxx
  #vterm-ctrl altscreen off
  vterm-ctrl altscreen on       #   on = Send C1 control characters as single 8-bit characters
  vterm-ctrl s8c1t on
}

rand() {
  LC_ALL=C tr -dc 1-9 </dev/urandom |
    dd ibs=1 obs=1 count=5 2>/dev/null
}

cols() {
  # print 16 color palette
  for i in 1 2 3 4 5 6; do
    printf '\033[4%sm  \033[m' "$i"
    printf '\033[10%sm  \033[m' "$i"
  done
  printf '\n\n'
}

main() {
  [ -f "$1" ] || {
    printf 'no palettes found in dir\n'
    exit 1
  }
  sp() {
    shift "$(($(rand) % $#))"
    printf "\n%s\n" "${1##*/}"
    paleta <"$1"
    cols
  }
  sp "$@"
}

cmd="~/.bin/sb -n '$SESS' /usr/bin/env bash -i 2>/dev/null; ~/.bin/sb -e '^q' -a '$SESS'"
acmd="asciinema rec -y --stdin --title 'ZSHELL ok' -c '$cmd'"
reset
#rand_palette "$@"
paleta < ~/.palettes/frontend-galaxy-dark
eval "$acmd"
