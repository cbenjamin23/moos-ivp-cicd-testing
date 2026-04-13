/************************************************************/
/*    NAME: Charles Benjamin                                */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: TrafficManager.cpp                              */
/************************************************************/

#include <algorithm>
#include <cmath>
#include <iterator>
#include <sstream>
#include "ACTable.h"
#include "MBUtils.h"
#include "AngleUtils.h"
#include "GeomUtils.h"
#include "TrafficManager.h"

using namespace std;

//---------------------------------------------------------
// Constructor

TrafficManager::TrafficManager() = default;

//---------------------------------------------------------
// Procedure: OnNewMail

bool TrafficManager::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  for(MOOSMSG_LIST::iterator p = NewMail.begin(); p != NewMail.end(); ++p) {
    CMOOSMsg &msg = *p;
    string key = msg.GetKey();

    if(key == "NODE_REPORT" && msg.IsString())
      handleNodeReport(msg.GetString());
    else if(key == "DEPLOY_ALL") {
      if(msg.IsDouble())
        m_deployed = (msg.GetDouble() != 0);
      else if(msg.IsString())
        m_deployed = (tolower(msg.GetString()) == "true");
    }
    else if(strBegins(key, "WPT_STAT_") && msg.IsString())
      handleWaypointStatus(key, msg.GetString());
    else if(key != "APPCAST_REQ")
      reportRunWarning("Unhandled Mail: " + key);
  }

  return(true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer

bool TrafficManager::OnConnectToServer()
{
  registerVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: Iterate

bool TrafficManager::Iterate()
{
  AppCastingMOOSApp::Iterate();

  maybeAssignReadyVehicles();
  postVisuals();

  Notify("TRAFFIC_BATCHES", (double)(m_batch_count));
  Notify("TRAFFIC_ASSIGN_FAILS", (double)(m_assign_failures));
  Notify("TRAFFIC_ASSIGN_MODE", m_last_mode);

  AppCastingMOOSApp::PostReport();
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp

bool TrafficManager::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  STRING_LIST sParams;
  m_MissionReader.EnableVerbatimQuoting(false);
  if(m_MissionReader.GetConfiguration(GetAppName(), sParams)) {
    for(STRING_LIST::iterator p = sParams.begin(); p != sParams.end(); ++p) {
      string orig = *p;
      string line = *p;
      string param = tolower(biteStringX(line, '='));
      string value = stripBlankEnds(line);

      if(param == "vnames") {
        m_vnames = parseString(value, ':');
      }
      else if(param == "center_x" && isNumber(value))
        m_center_x = atof(value.c_str());
      else if(param == "center_y" && isNumber(value))
        m_center_y = atof(value.c_str());
      else if(param == "base_radius" && isNumber(value))
        m_base_radius = atof(value.c_str());
      else if(param == "radius_jitter" && isNumber(value))
        m_radius_jitter = atof(value.c_str());
      else if(param == "min_target_sep" && isNumber(value))
        m_min_target_sep = atof(value.c_str());
      else if(param == "min_leg_len" && isNumber(value))
        m_min_leg_len = atof(value.c_str());
      else if(param == "min_pred_cpa" && isNumber(value))
        m_min_pred_cpa = atof(value.c_str());
      else if(param == "seed" && isNumber(value))
        m_seed = (unsigned int)(atoi(value.c_str()));
      else if(param == "max_tries" && isNumber(value))
        m_max_tries = (unsigned int)(atoi(value.c_str()));
      else if(param == "repost_interval" && isNumber(value))
        m_repost_interval = atof(value.c_str());
      else
        reportUnhandledConfigWarning(orig);
    }
  }

  m_rng.seed(m_seed);

  for(unsigned int i = 0; i < m_vnames.size(); ++i)
    m_states[m_vnames[i]] = TrafficVehicleState();

  registerVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: registerVariables

void TrafficManager::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("NODE_REPORT", 0);
  Register("DEPLOY_ALL", 0);

  for(unsigned int i = 0; i < m_vnames.size(); ++i)
    Register(statusVarName(m_vnames[i]), 0);
}

//---------------------------------------------------------
// Procedure: buildReport

bool TrafficManager::buildReport()
{
  m_msgs << "seed=" << m_seed << ", deployed=" << boolToString(m_deployed) << endl;
  m_msgs << "batches=" << m_batch_count << ", assign_failures=" << m_assign_failures
         << ", last_mode=" << m_last_mode << endl;

  ACTable actab(5);
  actab << "vname | node | ready | x | y";
  actab.addHeaderLines();
  for(unsigned int i = 0; i < m_vnames.size(); ++i) {
    const string& vname = m_vnames[i];
    const TrafficVehicleState& state = m_states[vname];
    actab << vname
          << boolToString(state.have_node)
          << boolToString(state.ready)
          << doubleToStringX(state.x, 1)
          << doubleToStringX(state.y, 1);
  }
  m_msgs << actab.getFormattedString();
  return(true);
}

//---------------------------------------------------------
// Procedure: handleNodeReport

bool TrafficManager::handleNodeReport(const string& sval)
{
  string vname = tokStringParse(sval, "NAME", ',', '=');
  if(vname == "")
    vname = tokStringParse(sval, "VNAME", ',', '=');
  if(vname == "")
    return(false);

  if(m_states.count(vname) == 0)
    return(false);

  string sx = tokStringParse(sval, "X", ',', '=');
  string sy = tokStringParse(sval, "Y", ',', '=');
  string sspd = tokStringParse(sval, "SPD", ',', '=');
  if(!isNumber(sx) || !isNumber(sy))
    return(false);

  TrafficVehicleState& state = m_states[vname];
  state.x = atof(sx.c_str());
  state.y = atof(sy.c_str());
  if(isNumber(sspd))
    state.speed = atof(sspd.c_str());
  state.have_node = true;
  return(true);
}

//---------------------------------------------------------
// Procedure: handleWaypointStatus

void TrafficManager::handleWaypointStatus(const string& key, const string& sval)
{
  string rest = key.substr(string("WPT_STAT_").length());
  string vname = "";
  for(unsigned int i = 0; i < m_vnames.size(); ++i) {
    string up = upperName(m_vnames[i]);
    if((rest == up) || (rest == (up + "_" + up))) {
      vname = m_vnames[i];
      break;
    }
  }

  if((vname == "") || (m_states.count(vname) == 0))
    return;

  string lower = tolower(sval);
  if(strContains(lower, "completed") || strContains(lower, "cycled")) {
    m_states[vname].ready = true;
    m_active_assigns.erase(vname);
    postWaypointMarker(vname, 0, 0, false);
  }
}

//---------------------------------------------------------
// Procedure: maybeAssignReadyVehicles

bool TrafficManager::maybeAssignReadyVehicles()
{
  if(!m_deployed)
    return(false);
  if(!allVehiclesKnown())
    return(false);

  double now = MOOSTime();
  if(!m_active_assigns.empty() && ((now - m_last_post_time) >= m_repost_interval)) {
    for(map<string, pair<double, double> >::const_iterator p = m_active_assigns.begin();
        p != m_active_assigns.end(); ++p) {
      string var = updateVarName(p->first);
      string value = "point=" + doubleToStringX(p->second.first, 2) + ",";
      value += doubleToStringX(p->second.second, 2);
      Notify(var, value);
    }
    m_last_post_time = now;
  }

  bool assigned_any = false;
  for(unsigned int i = 0; i < m_vnames.size(); ++i) {
    assigned_any = assignVehicle(m_vnames[i]) || assigned_any;
  }

  return(assigned_any);
}

//---------------------------------------------------------
// Procedure: allVehiclesReady / allVehiclesKnown

bool TrafficManager::allVehiclesKnown() const
{
  for(unsigned int i = 0; i < m_vnames.size(); ++i) {
    map<string, TrafficVehicleState>::const_iterator p = m_states.find(m_vnames[i]);
    if((p == m_states.end()) || !p->second.have_node)
      return(false);
  }
  return(true);
}

//---------------------------------------------------------
// Procedure: assignVehicle / generateAssignment

bool TrafficManager::assignVehicle(const string& vname)
{
  map<string, TrafficVehicleState>::iterator p = m_states.find(vname);
  if((p == m_states.end()) || !p->second.have_node || !p->second.ready)
    return(false);

  double tx = 0;
  double ty = 0;
  if(!generateAssignment(vname, tx, ty)) {
    m_assign_failures++;
    return(false);
  }

  string var = updateVarName(vname);
  string value = "point=" + doubleToStringX(tx, 2) + ",";
  value += doubleToStringX(ty, 2);
  Notify(var, value);
  postWaypointMarker(vname, tx, ty, true);

  p->second.ready = false;
  m_active_assigns[vname] = make_pair(tx, ty);
  m_batch_count++;
  m_last_mode = "free";
  m_last_post_time = MOOSTime();
  return(true);
}

bool TrafficManager::generateAssignment(const string& vname, double& tx, double& ty)
{
  uniform_real_distribution<double> angle_roll(0.0, 360.0);
  uniform_real_distribution<double> radial_jitter(-m_radius_jitter, m_radius_jitter);

  for(unsigned int tries = 0; tries < m_max_tries; ++tries) {
    double ang = angle_roll(m_rng);
    double rad = m_base_radius + radial_jitter(m_rng);
    projectPoint(angle360(90.0 - ang), rad, m_center_x, m_center_y, tx, ty);

    if(!targetSpacedEnough(vname, tx, ty))
      continue;
    if(!assignmentLongEnough(vname, tx, ty))
      continue;
    if(!assignmentSafeEnough(vname, tx, ty))
      continue;
    return(true);
  }

  return(false);
}

bool TrafficManager::targetSpacedEnough(const string& vname, double tx, double ty) const
{
  for(map<string, pair<double, double> >::const_iterator p = m_active_assigns.begin();
      p != m_active_assigns.end(); ++p) {
    if(p->first == vname)
      continue;
    if(distPointToPoint(tx, ty, p->second.first, p->second.second) < m_min_target_sep)
      return(false);
  }
  return(true);
}

bool TrafficManager::assignmentLongEnough(const string& vname, double tx, double ty) const
{
  map<string, TrafficVehicleState>::const_iterator p = m_states.find(vname);
  if(p == m_states.end())
    return(false);
  return(distPointToPoint(tx, ty, p->second.x, p->second.y) >= m_min_leg_len);
}

bool TrafficManager::assignmentSafeEnough(const string& vname, double tx, double ty) const
{
  map<string, TrafficVehicleState>::const_iterator p1 = m_states.find(vname);
  if(p1 == m_states.end())
    return(false);

  for(unsigned int i = 0; i < m_vnames.size(); ++i) {
    const string& other = m_vnames[i];
    if(other == vname)
      continue;

    map<string, TrafficVehicleState>::const_iterator p2 = m_states.find(other);
    if((p2 == m_states.end()) || !p2->second.have_node)
      continue;

    map<string, pair<double, double> >::const_iterator ap = m_active_assigns.find(other);
    if(ap == m_active_assigns.end())
      continue;

    double pred_cpa = pairPredictedCPA(p1->second, tx, ty,
                                       p2->second, ap->second.first, ap->second.second);
    if(pred_cpa < m_min_pred_cpa)
      return(false);
  }

  return(true);
}

//---------------------------------------------------------
// Procedure: upperName / pairPredictedCPA

double TrafficManager::pairPredictedCPA(const TrafficVehicleState& state1,
                                          double tx1, double ty1,
                                          const TrafficVehicleState& state2,
                                          double tx2, double ty2) const
{
  double dx1 = tx1 - state1.x;
  double dy1 = ty1 - state1.y;
  double dx2 = tx2 - state2.x;
  double dy2 = ty2 - state2.y;
  double leg1 = hypot(dx1, dy1);
  double leg2 = hypot(dx2, dy2);
  if((leg1 <= 0) || (leg2 <= 0))
    return(0);

  double spd1 = state1.speed;
  double spd2 = state2.speed;
  if(spd1 < 0.5)
    spd1 = 1.8;
  if(spd2 < 0.5)
    spd2 = 1.8;

  double vx1 = (dx1 / leg1) * spd1;
  double vy1 = (dy1 / leg1) * spd1;
  double vx2 = (dx2 / leg2) * spd2;
  double vy2 = (dy2 / leg2) * spd2;
  double t1 = leg1 / spd1;
  double t2 = leg2 / spd2;

  double min_dist = intervalMinDist(state1.x - state2.x,
                                    state1.y - state2.y,
                                    vx1 - vx2,
                                    vy1 - vy2,
                                    0, min(t1, t2));

  if(t1 < t2) {
    double rx = tx1 - state2.x;
    double ry = ty1 - state2.y;
    min_dist = min(min_dist, intervalMinDist(rx, ry, -vx2, -vy2, t1, t2));
  }
  else if(t2 < t1) {
    double rx = state1.x - tx2;
    double ry = state1.y - ty2;
    min_dist = min(min_dist, intervalMinDist(rx, ry, vx1, vy1, t2, t1));
  }

  min_dist = min(min_dist, hypot(tx1 - tx2, ty1 - ty2));
  return(min_dist);
}

double TrafficManager::intervalMinDist(double rx, double ry,
                                         double rvx, double rvy,
                                         double t0, double t1) const
{
  if(t1 < t0)
    return(hypot(rx + (rvx * t0), ry + (rvy * t0)));

  double rv_sq = (rvx * rvx) + (rvy * rvy);
  double t_star = t0;
  if(rv_sq > 0) {
    t_star = -((rx * rvx) + (ry * rvy)) / rv_sq;
    if(t_star < t0)
      t_star = t0;
    if(t_star > t1)
      t_star = t1;
  }

  double dx = rx + (rvx * t_star);
  double dy = ry + (rvy * t_star);
  return(hypot(dx, dy));
}

string TrafficManager::upperName(const string& vname) const
{
  return(toupper(vname));
}

string TrafficManager::statusVarName(const string& vname) const
{
  string up = upperName(vname);
  return("WPT_STAT_" + up);
}

string TrafficManager::updateVarName(const string& vname) const
{
  string up = upperName(vname);
  return("WPT_UPDATE_" + up + "_ALL");
}

void TrafficManager::postWaypointMarker(const string& vname, double tx, double ty,
                                          bool active)
{
  string spec = "x=" + doubleToStringX(tx, 2);
  spec += ",y=" + doubleToStringX(ty, 2);
  spec += ",label=" + vname + "_next_waypoint";
  spec += ",type=next_waypoint";
  spec += ",source=pTrafficManager";
  spec += ",vertex_color=yellow";
  spec += ",label_color=white";
  spec += ",vertex_size=4";
  spec += ",active=" + boolToString(active);
  Notify("VIEW_POINT", spec);
}

void TrafficManager::postVisuals()
{
  double now = MOOSTime();
  if((now - m_last_ring_post_time) < 5.0)
    return;

  string pts = "pts={";
  unsigned int samples = 36;
  for(unsigned int i = 0; i <= samples; ++i) {
    double ang = 360.0 * ((double)(i) / (double)(samples));
    double x = 0;
    double y = 0;
    projectPoint(angle360(90.0 - ang), m_base_radius, m_center_x, m_center_y, x, y);
    if(i > 0)
      pts += ":";
    pts += doubleToStringX(x, 2) + "," + doubleToStringX(y, 2);
  }
  pts += "}";

  string spec = pts;
  spec += ",label=traffic_ring";
  spec += ",edge_color=gray70";
  spec += ",vertex_color=invisible";
  spec += ",label_color=invisible";
  spec += ",edge_size=2";
  Notify("VIEW_SEGLIST", spec);
  m_last_ring_post_time = now;
}
