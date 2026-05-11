#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Mission: uquerydb_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------

ME=`basename "$0"`
DIR="$(cd "$(dirname "$0")" && pwd)"
exec "$DIR/../../../harnesses/uquerydb_harnesses/H01-uquerydb_unit/zlaunch.sh" "$@"
