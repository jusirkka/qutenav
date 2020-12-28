#include "cm93reader.h"
#include <QFile>
#include <QDataStream>
#include <functional>
#include <QDate>
#include "s52presentation.h"



const QString& CM93Reader::name() const {
  return m_name;
}

const GeoProjection* CM93Reader::geoprojection() const {
  return m_proj;
}

CM93Reader::CM93Reader()
  : m_name("cm93")
  , m_proj(GeoProjection::CreateProjection("CM93Mercator"))
{}


S57ChartOutline CM93Reader::readOutline(const QString& path) const {
  // TODO
  return S57ChartOutline();
}



void CM93Reader::readChart(GL::VertexVector& vertices,
                           GL::IndexVector& indices,
                           S57::ObjectVector& objects,
                           const QString& path,
                           const GeoProjection* proj) const {
  // TODO
}
