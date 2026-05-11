#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Mission: upokedb_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------
HARNESS_DIR="$(cd "$(dirname "$0")/../../../harnesses/upokedb_harnesses/H01-upokedb_unit" && pwd)"
exec "$HARNESS_DIR/zlaunch.sh" "$@"
