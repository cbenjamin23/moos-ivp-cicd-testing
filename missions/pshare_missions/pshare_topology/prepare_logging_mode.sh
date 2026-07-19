#!/bin/bash
# Prepare generated nsplug sidecars for one public logging mode.

set -u

ME=$(basename "$0")
LOG_MODE="${1:-minimal}"

case "$LOG_MODE" in
    minimal|full) ;;
    *) echo "$ME: --log must be minimal or full" >&2; exit 2 ;;
esac

rm -f meta_shoreside.moosx meta_alpha.moosx meta_bravo.moosx meta_relay.moosx
if [ "$LOG_MODE" = full ]; then
    nspatch --stem=meta_shoreside.moos full-logging-shoreside.xmoos \
        --targ=meta_shoreside.moosx || exit 1
    nspatch --stem=meta_alpha.moos full-logging-alpha.xmoos \
        --targ=meta_alpha.moosx || exit 1
    nspatch --stem=meta_bravo.moos full-logging-bravo.xmoos \
        --targ=meta_bravo.moosx || exit 1
    nspatch --stem=meta_relay.moos full-logging-relay.xmoos \
        --targ=meta_relay.moosx || exit 1
fi
