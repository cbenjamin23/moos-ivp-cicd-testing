#pragma once

#include <memory>
#include <vector>

#include "IvPBox.h"
#include "IvPDomain.h"
#include "PDMap.h"

inline IvPDomain makeCourseSpeedDomain()
{
  // Standard pHelmIvP decision space used by ZAIC, AOF, and solver tests.
  IvPDomain domain;
  domain.addDomain("course", 0, 359, 360);
  domain.addDomain("speed", 0, 5, 6);
  return domain;
}

inline IvPDomain makeXYDomain()
{
  IvPDomain domain;
  domain.addDomain("x", 0, 100, 101);
  domain.addDomain("y", 0, 100, 101);
  return domain;
}

inline IvPBox makePointBox(unsigned int dim, const std::vector<int>& pts)
{
  // Solver and encoder tests evaluate utility at discrete domain indices, not
  // continuous navigation values.
  IvPBox box(dim);
  for(unsigned int i = 0; i < dim; ++i)
    box.setPTS(i, pts[i], pts[i]);
  return box;
}

inline IvPBox makeOneDimPoint(unsigned int pt)
{
  IvPBox box(1);
  box.setPTS(0, pt, pt);
  return box;
}

inline double evalPDMapAtIndex(const PDMap& pdmap, unsigned int pt)
{
  // One-dimensional PDMap checks use point boxes to exercise the same eval
  // path used after an IvP function is decoded.
  IvPBox box = makeOneDimPoint(pt);
  return pdmap.evalPoint(&box);
}
