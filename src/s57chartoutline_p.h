#pragma once

#include <QSharedData>
#include <QPoint>
#include "types.h"
#include <QDate>

using Region = QVector<WGS84PointVector>;

struct S57ChartOutlinePrivate: public QSharedData {


  inline S57ChartOutlinePrivate()
  {}

  inline S57ChartOutlinePrivate(const S57ChartOutlinePrivate& d)
    : QSharedData(d)
    , extent(d.extent.sw(), d.extent.se(), d.extent.nw(), d.extent.ne())
    , cov(d.cov)
    , nocov(d.nocov)
    , center(d.center)
    , scaling(d.scaling)
    , scale(d.scale)
    , pub(d.pub)
    , mod(d.mod) {}

  Extent extent;
  Region cov;
  Region nocov;
  WGS84Point center;
  QSizeF scaling;
  quint32 scale;
  QDate pub;
  QDate mod;
};
