#pragma once

#include "s57chartoutline.h"
#include "s57object.h"
#include "geoprojection.h"

class ChartFileReader {
public:

  virtual S57ChartOutline readOutline(const QString& path) const = 0;

  virtual void readChart(GL::VertexVector& vertices,
                         GL::IndexVector& indices,
                         S57::ObjectVector& objects,
                         const QString& path,
                         const GeoProjection* proj) const = 0;

  virtual const QString& name() const = 0;

  virtual const GeoProjection* geoprojection() const = 0;

  virtual ~ChartFileReader() = default;

protected:
  static QRectF computeBBox(const S57::ElementDataVector &elems,
                            const GL::VertexVector& vertices,
                            const GL::IndexVector& indices);

  static QPointF computeLineCenter(const S57::ElementDataVector &elems,
                                   const GL::VertexVector& vertices,
                                   const GL::IndexVector& indices);

};

