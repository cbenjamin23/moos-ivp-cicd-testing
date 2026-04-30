#pragma once

#include <string>

#include "XYPolygon.h"
#include "XYSegList.h"

inline XYPolygon makeSquarePoly(double low_x, double low_y, double high_x, double high_y)
{
  // Geometry tests use axis-aligned fixtures when the behavior under test is
  // parsing, clipping, or containment rather than polygon construction.
  XYPolygon poly;
  poly.add_vertex(low_x, low_y);
  poly.add_vertex(high_x, low_y);
  poly.add_vertex(high_x, high_y);
  poly.add_vertex(low_x, high_y);
  return poly;
}

inline XYSegList makeTransitLane()
{
  // A short mission-like lane gives path tests a repeatable survey/transit
  // route without importing a full shoreside mission.
  XYSegList lane;
  lane.add_vertex(0, 0);
  lane.add_vertex(50, 0);
  lane.add_vertex(100, 25);
  return lane;
}

inline bool stringContains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}
