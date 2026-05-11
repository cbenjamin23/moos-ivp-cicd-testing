#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Mission: utermcommand_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------

DIR="$(cd "$(dirname "$0")" && pwd)"
exec "$DIR/../../../harnesses/utermcommand_harnesses/H01-utermcommand_unit/zlaunch.sh" "$@"
