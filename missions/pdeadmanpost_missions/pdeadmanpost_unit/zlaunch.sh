#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Mission: pdeadmanpost_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------

DIR="$(cd "$(dirname "$0")" && pwd)"
exec "$DIR/../../../harnesses/pdeadmanpost_harnesses/H01-pdeadmanpost_unit/zlaunch.sh" "$@"
