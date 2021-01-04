#pragma once

#include "detailmode.h"

class ChartMode: public DetailMode
{
public:
  ChartMode(float wmm, float hmm, GeoProjection* p);
  DetailMode* largerScaleMode() const override;
  DetailMode* smallerScaleMode() const override;
  bool hasCharts() const;

  ~ChartMode();

protected:

  QString className() const override {return "ChartMode";}

};

