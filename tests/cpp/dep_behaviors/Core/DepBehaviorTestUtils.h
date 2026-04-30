#pragma once

#include <string>
#include <vector>

#include "InfoBuffer.h"
#include "IvPBox.h"
#include "IvPDomain.h"
#include "LedgerSnap.h"
#include "VarDataPair.h"

inline IvPDomain makeDepBehaviorDomain()
{
  // Deprecated behavior tests still use the current helm course/speed domain
  // so old behavior output can be compared against modern utility surfaces.
  IvPDomain domain;
  domain.addDomain("course", 0, 359, 360);
  domain.addDomain("speed", 0, 5, 6);
  return domain;
}

inline IvPBox makeCourseSpeedEvalBox(unsigned int course, unsigned int speed)
{
  IvPBox box(2);
  box.setPTS(0, course, course);
  box.setPTS(1, speed, speed);
  return box;
}

inline const VarDataPair* findDepBehaviorMessage(const std::vector<VarDataPair>& messages,
                                                 const std::string& var)
{
  for(const auto& message : messages) {
    if(message.get_var() == var)
      return &message;
  }
  return nullptr;
}

inline const VarDataPair* findDepBehaviorMessageByKey(const std::vector<VarDataPair>& messages,
                                                      const std::string& var,
                                                      const std::string& key)
{
  // Some legacy behaviors append metadata to message keys, so tests accept
  // either an exact key or the same key at the end of the posted string.
  for(const auto& message : messages) {
    const std::string message_key = message.get_key();
    const bool exact_key = (message_key == key);
    const bool suffix_key =
      (message_key.size() >= key.size()) &&
      (message_key.compare(message_key.size() - key.size(), key.size(), key) == 0);
    if((message.get_var() == var) && (exact_key || suffix_key))
      return &message;
  }
  return nullptr;
}

inline bool depTextContains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

inline void seedDepOwnship(InfoBuffer& info,
                           double x,
                           double y,
                           double heading,
                           double speed)
{
  info.setValue("NAV_X", x);
  info.setValue("NAV_Y", y);
  info.setValue("NAV_HEADING", heading);
  info.setValue("NAV_SPEED", speed);
}

inline void seedDepContact(LedgerSnap& ledger,
                           const std::string& contact,
                           double x,
                           double y,
                           double heading,
                           double speed,
                           double utc)
{
  ledger.setX(contact, x);
  ledger.setY(contact, y);
  ledger.setHdg(contact, heading);
  ledger.setSpd(contact, speed);
  ledger.setUTC(contact, utc);
}

inline std::string depObstaclePolySpec()
{
  // Compact obstacle polygon used by deprecated obstacle behaviors without
  // needing a mission file or obstacle manager.
  return "pts={40,-10:60,-10:60,10:40,10},label=obstacle_alpha";
}
