#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "IvPDomain.h"
#include "LedgerSnap.h"
#include "VarDataPair.h"

inline IvPDomain makeBehaviorDomain()
{
  // Behavior unit tests use the standard helm decision space without needing a
  // full pHelmIvP launch.
  IvPDomain domain;
  domain.addDomain("course", 0, 359, 360);
  domain.addDomain("speed", 0, 5, 6);
  return domain;
}

inline bool containsText(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

inline const VarDataPair* findMessage(const std::vector<VarDataPair>& messages,
                                      const std::string& var)
{
  // Behavior tests assert on posted MOOS variables directly, matching how
  // pHelmIvP would expose warnings, flags, and status updates.
  for(const auto& message : messages) {
    if(message.get_var() == var)
      return &message;
  }
  return nullptr;
}

inline void seedLedgerContact(LedgerSnap& ledger,
                              const std::string& contact,
                              double x,
                              double y,
                              double heading,
                              double speed,
                              double utc = 0)
{
  // Contact behaviors consume LedgerSnap fields from the contact ledger; this
  // keeps fixtures focused on the geometry being tested.
  ledger.setX(contact, x);
  ledger.setY(contact, y);
  ledger.setHdg(contact, heading);
  ledger.setSpd(contact, speed);
  ledger.setUTC(contact, utc);
}
