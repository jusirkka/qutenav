#pragma once
#include "chartfilereader.h"

class CM93Reader: public  ChartFileReader {

  friend class ChartFileReader;

public:

  S57ChartOutline readOutline(const QString& path) const override;

  void readChart(GL::VertexVector& vertices,
                 GL::IndexVector& indices,
                 S57::ObjectVector& objects,
                 const QString& path,
                 const GeoProjection* proj) const override;

  const QString& name() const override;

  const GeoProjection* geoprojection() const override;

private:

  CM93Reader();

  QString m_name;
  GeoProjection* m_proj;
};

