/************************************************************/
/*    NAME: Charles Benjamin                                */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: TrafficManager_Info.cpp                         */
/************************************************************/

#include <cstdlib>
#include <iostream>
#include "ColorParse.h"
#include "ReleaseInfo.h"
#include "TrafficManager_Info.h"

using namespace std;

void showSynopsis()
{
  blk("SYNOPSIS:");
  blk("------------------------------------");
  blk("  pTrafficManager assigns fresh waypoint targets to a fixed");
  blk("  vehicle set on a ring so multi-vessel COLREGS traffic missions");
  blk("  can run continuously under seeded randomization.");
}

void showHelpAndExit()
{
  cout << endl;
  showSynopsis();
  blk("");
  blk("Usage: pTrafficManager file.moos [OPTIONS] [time_warp]");
  blk("");
  blk("Options:");
  blk("  --alias=<ProcessName>");
  blk("  --example, -e");
  blk("  --help, -h");
  blk("  --interface, -i");
  blk("  --version,-v");
  cout << endl;
  exit(0);
}

void showExampleConfigAndExit()
{
  blk("ProcessConfig = pTrafficManager");
  blk("{");
  blk("  AppTick        = 2");
  blk("  CommsTick      = 2");
  blk("  vnames         = abe:ben:cal:deb:eve");
  blk("  center_x       = 23");
  blk("  center_y       = -82");
  blk("  base_radius    = 34");
  blk("  radius_jitter  = 2");
  blk("  min_target_sep = 12");
  blk("  min_leg_len    = 18");
  blk("  min_pred_cpa   = 8");
  blk("  seed           = 17");
  blk("  max_tries      = 250");
  blk("  repost_interval = 10");
  blk("}");
  exit(0);
}

void showInterfaceAndExit()
{
  blk("SUBSCRIPTIONS:");
  blk("  NODE_REPORT");
  blk("  DEPLOY_ALL");
  blk("  WPT_STAT_<UP_VNAME>");
  blk("");
  blk("PUBLICATIONS:");
  blk("  WPT_UPDATE_<UP_VNAME>");
  blk("  TRAFFIC_BATCHES");
  blk("  TRAFFIC_ASSIGN_FAILS");
  blk("  TRAFFIC_ASSIGN_MODE");
  exit(0);
}

void showReleaseInfoAndExit()
{
  showReleaseInfo("pTrafficManager", "gpl");
  exit(0);
}
