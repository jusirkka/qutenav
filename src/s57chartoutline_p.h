#pragma once

#include <QSharedData>
#include <QPoint>
#include "types.h"
#include <QDate>

struct S57ChartOutlinePrivate: public QSharedData {


  inline S57ChartOutlinePrivate()
  {}

  inline S57ChartOutlinePrivate(const S57ChartOutlinePrivate& d)
    : QSharedData(d)
    , extent(d.extent.sw(), d.extent.se(), d.extent.nw(), d.extent.ne())
    , center(d.center)
    , scale(d.scale)
    , pub(d.pub)
    , mod(d.mod) {}

  Extent extent;
  WGS84Point center;
  quint32 scale;
  QDate pub;
  QDate mod;
};
