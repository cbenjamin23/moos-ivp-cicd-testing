#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Mission: pechovar_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------

DIR="$(cd "$(dirname "$0")" && pwd)"
exec "$DIR/../../../harnesses/pechovar_harnesses/H01-pechovar_unit/zlaunch.sh" "$@"
