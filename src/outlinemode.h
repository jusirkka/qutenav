#pragma once

#include "detailmode.h"

class OutlineMode: public DetailMode
{
public:
  OutlineMode(float wmm, float hmm, GeoProjection* p);
  DetailMode* largerScaleMode() const override;
  DetailMode* smallerScaleMode() const override;
  bool hasCharts() const override;

  ~OutlineMode();

protected:

  QString className() const override {return "OutlineMode";}

};
