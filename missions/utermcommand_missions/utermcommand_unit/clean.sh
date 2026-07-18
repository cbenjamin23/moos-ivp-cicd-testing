#!/bin/bash
#------------------------------------------------------------
#   Script: clean.sh
#  Mission: utermcommand_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------

rm -f targ_*.moos
rm -f termcommand.moos termcommand.out eval_not_ready.out eval_ready.out launch.out results.txt
rm -rf LOG_* XLOG_* MOOSLog_*
