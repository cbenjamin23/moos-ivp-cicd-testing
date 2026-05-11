#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Mission: pspoofnode_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------

DIR="$(cd "$(dirname "$0")" && pwd)"
exec "$DIR/../../../harnesses/pspoofnode_harnesses/H01-pspoofnode_unit/zlaunch.sh" "$@"
