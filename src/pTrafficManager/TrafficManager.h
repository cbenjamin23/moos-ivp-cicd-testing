/************************************************************/
/*    NAME: Charles Benjamin                                */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: TrafficManager.h                                */
/************************************************************/

#ifndef TRAFFIC_MANAGER_HEADER
#define TRAFFIC_MANAGER_HEADER

#include <map>
#include <random>
#include <string>
#include <vector>
#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"

struct TrafficVehicleState {
  bool   have_node = false;
  bool   ready = true;
  double x = 0;
  double y = 0;
  double speed = 0;
};

class TrafficManager : public AppCastingMOOSApp
{
 public:
  TrafficManager();
  ~TrafficManager() override = default;

 protected:
  bool OnNewMail(MOOSMSG_LIST &NewMail) override;
  bool Iterate() override;
  bool OnConnectToServer() override;
  bool OnStartUp() override;
  bool buildReport() override;

 protected:
  void registerVariables();
  bool handleNodeReport(const std::string& sval);
  void handleWaypointStatus(const std::string& key, const std::string& sval);
  bool maybeAssignReadyVehicles();
  bool assignVehicle(const std::string& vname);
  bool allVehiclesKnown() const;
  bool generateAssignment(const std::string& vname, double& tx, double& ty);
  bool targetSpacedEnough(const std::string& vname, double tx, double ty) const;
  bool assignmentLongEnough(const std::string& vname, double tx, double ty) const;
  bool assignmentSafeEnough(const std::string& vname, double tx, double ty) const;
  double pairPredictedCPA(const TrafficVehicleState& state1,
                          double tx1, double ty1,
                          const TrafficVehicleState& state2,
                          double tx2, double ty2) const;
  double intervalMinDist(double rx, double ry,
                         double rvx, double rvy,
                         double t0, double t1) const;
  std::string upperName(const std::string& vname) const;
  std::string statusVarName(const std::string& vname) const;
  std::string updateVarName(const std::string& vname) const;
  void postWaypointMarker(const std::string& vname, double tx, double ty, bool active);
  void postVisuals();

 private:
  std::vector<std::string> m_vnames;
  std::map<std::string, TrafficVehicleState> m_states;

  bool m_deployed = false;

  double m_center_x = 23.0;
  double m_center_y = -82.0;
  double m_base_radius = 32.0;
  double m_radius_jitter = 2.0;
  double m_min_target_sep = 12.0;
  double m_min_leg_len = 18.0;
  double m_min_pred_cpa = 8.0;
  unsigned int m_seed = 17;
  unsigned int m_max_tries = 250;
  double m_repost_interval = 10.0;

  std::mt19937 m_rng;
  std::map<std::string, std::pair<double, double> > m_active_assigns;
  double m_last_post_time = 0;
  double m_last_ring_post_time = 0;

  unsigned int m_batch_count = 0;
  unsigned int m_assign_failures = 0;
  std::string  m_last_mode = "free";
};

#endif
