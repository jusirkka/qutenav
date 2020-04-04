#pragma once

#include "detailmode.h"

class OutlineMode: public DetailMode
{
public:
  OutlineMode(float wmm, float hmm);
  DetailMode* largerScaleMode(float wmm, float hmm) const override;
  DetailMode* smallerScaleMode(float wmm, float hmm) const override;

protected:

  QString className() const override {return "OutlineMode";}

};
