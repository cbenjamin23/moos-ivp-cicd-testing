#!/bin/bash
#------------------------------------------------------------
#   Script: clean.sh
#  Mission: uquerydb_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------

rm -f targ_*.moos
rm -f .checkvars
rm -f querydb.out mayfinish.out launch.out readiness.out
rm -f query_connect.moos query_config.moos results.txt
rm -rf LOG_* XLOG_* MOOSLog_*
