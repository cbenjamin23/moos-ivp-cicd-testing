#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Mission: psearchgrid_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------

DIR="$(cd "$(dirname "$0")" && pwd)"
exec "$DIR/../../../harnesses/psearchgrid_harnesses/H01-psearchgrid_unit/zlaunch.sh" "$@"
