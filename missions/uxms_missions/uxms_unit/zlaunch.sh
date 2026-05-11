#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Mission: uxms_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------
HARNESS_DIR="$(cd "$(dirname "$0")/../../../harnesses/uxms_harnesses/H01-uxms_unit" && pwd)"
exec "$HARNESS_DIR/zlaunch.sh" "$@"

