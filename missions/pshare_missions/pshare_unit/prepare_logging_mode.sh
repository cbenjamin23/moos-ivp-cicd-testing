#!/bin/bash
# Prepare generated nsplug sidecars for one public logging mode.

set -u

ME=$(basename "$0")
LOG_MODE="${1:-minimal}"

case "$LOG_MODE" in
    minimal|full) ;;
    *) echo "$ME: --log must be minimal or full" >&2; exit 2 ;;
esac

rm -f meta_shoreside.moosx meta_peer.moosx
if [ "$LOG_MODE" = full ]; then
    nspatch --stem=meta_shoreside.moos full-logging-shoreside.xmoos \
        --targ=meta_shoreside.moosx || exit 1
    nspatch --stem=meta_peer.moos full-logging-peer.xmoos \
        --targ=meta_peer.moosx || exit 1
fi
