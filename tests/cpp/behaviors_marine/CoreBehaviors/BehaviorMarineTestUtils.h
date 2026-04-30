#pragma once

#include <string>
#include <vector>

#include "IvPBuildTestUtils.h"
#include "IvPFunction.h"
#include "LedgerSnap.h"
#include "VarDataPair.h"

inline bool findBehaviorMessage(const std::vector<VarDataPair>& messages,
                                const std::string& var,
                                VarDataPair& found)
{
  // Marine behavior tests inspect the same outgoing posts the helm would send
  // to MOOSDB, especially warnings and behavior-specific flags.
  for(const VarDataPair& message : messages) {
    if(message.get_var() == var) {
      found = message;
      return true;
    }
  }
  return false;
}

inline double evalOneDimPoint(IvPFunction& ipf, unsigned int point)
{
  IvPBox box = makeOneDimPoint(point);
  return ipf.getPDMap()->evalPoint(&box);
}

inline double evalTwoDimPoint(IvPFunction& ipf,
                              unsigned int first,
                              unsigned int second)
{
  IvPBox box = makePointBox(2, {static_cast<int>(first), static_cast<int>(second)});
  return ipf.getPDMap()->evalPoint(&box);
}

inline double evalCourseSpeedPoint(IvPFunction& ipf,
                                   unsigned int course,
                                   unsigned int speed)
{
  // Most marine behaviors emit course/speed IvP functions, so tests evaluate
  // representative discrete decisions instead of only checking non-null IPFs.
  return evalTwoDimPoint(ipf, course, speed);
}

inline IvPDomain makeDepthDomain()
{
  IvPDomain domain;
  domain.addDomain("depth", 0, 100, 101);
  return domain;
}

inline IvPDomain makeSpeedDepthDomain()
{
  IvPDomain domain;
  domain.addDomain("speed", 0, 5, 6);
  domain.addDomain("depth", 0, 100, 101);
  return domain;
}

inline void seedOwnshipContactInfo(InfoBuffer& info,
                                   LedgerSnap& ledger,
                                   const std::string& contact,
                                   double osx,
                                   double osy,
                                   double osh,
                                   double osv,
                                   double cnx,
                                   double cny,
                                   double cnh,
                                   double cnv)
{
  // Mirrors the runtime split between ownship NAV_* variables in InfoBuffer
  // and contact state in the behavior ledger.
  info.setValue("NAV_X", osx);
  info.setValue("NAV_Y", osy);
  info.setValue("NAV_HEADING", osh);
  info.setValue("NAV_SPEED", osv);

  ledger.setX(contact, cnx);
  ledger.setY(contact, cny);
  ledger.setHdg(contact, cnh);
  ledger.setSpd(contact, cnv);
  ledger.setUTC(contact, info.getCurrTime());
}
