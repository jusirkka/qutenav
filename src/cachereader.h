#pragma once

#include "chartfilereader.h"


class CacheReader: public ChartFileReader {

public:

  CacheReader();

  const GeoProjection* geoprojection() const override;
  GeoProjection* configuredProjection(const QString &path) const override;

  S57ChartOutline readOutline(const QString& path, const GeoProjection* gp) const override;

  void readChart(GL::VertexVector& vertices,
                 GL::IndexVector& indices,
                 S57::ObjectVector& objects,
                 const QString& path,
                 const GeoProjection* proj) const override;


  static QByteArray CacheId(const QString& path);

private:

  GeoProjection* m_proj;

};




