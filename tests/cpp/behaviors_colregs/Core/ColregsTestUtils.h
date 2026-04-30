#pragma once

#include <string>
#include <vector>

#include "InfoBuffer.h"
#include "IvPBox.h"
#include "IvPDomain.h"
#include "LedgerSnap.h"
#include "VarDataPair.h"

inline IvPDomain makeColregsDomain()
{
  // COLREGS behavior tests stay in the normal helm course/speed space so
  // generated avoidance functions are comparable to live pHelmIvP behavior.
  IvPDomain domain;
  domain.addDomain("course", 0, 359, 360);
  domain.addDomain("speed", 0, 5, 6);
  return domain;
}

inline IvPBox makeCourseSpeedBox(unsigned int course, unsigned int speed)
{
  IvPBox box(2);
  box.setPTS(0, course, course);
  box.setPTS(1, speed, speed);
  return box;
}

inline bool textContains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

inline const VarDataPair* findPosted(const std::vector<VarDataPair>& messages,
                                     const std::string& var)
{
  // Tests check posts by MOOS variable name rather than by vector position,
  // matching the unordered nature of behavior output.
  for(const auto& message : messages) {
    if(message.get_var() == var)
      return &message;
  }
  return nullptr;
}

inline void seedColregsOwnship(InfoBuffer& buffer,
                               double x,
                               double y,
                               double heading,
                               double speed)
{
  buffer.setValue("NAV_X", x);
  buffer.setValue("NAV_Y", y);
  buffer.setValue("NAV_HEADING", heading);
  buffer.setValue("NAV_SPEED", speed);
}

inline void seedColregsContact(LedgerSnap& ledger,
                               const std::string& contact,
                               double x,
                               double y,
                               double heading,
                               double speed,
                               double utc)
{
  // COLREGS scenarios depend on relative motion, so fixtures seed the same
  // ledger fields the contact manager would maintain at runtime.
  ledger.setX(contact, x);
  ledger.setY(contact, y);
  ledger.setHdg(contact, heading);
  ledger.setSpd(contact, speed);
  ledger.setUTC(contact, utc);
}
