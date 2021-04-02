#pragma once

#include "s57chartoutline.h"
#include "s57object.h"
#include "geoprojection.h"
#include <QtPlugin>

class ChartFileReader {
public:

  virtual const GeoProjection* geoprojection() const = 0;
  virtual GeoProjection* configuredProjection(const QString& path) const = 0;

  virtual S57ChartOutline readOutline(const QString& path,
                                      const GeoProjection* gp) const = 0;

  virtual void readChart(GL::VertexVector& vertices,
                         GL::IndexVector& indices,
                         S57::ObjectVector& objects,
                         const QString& path,
                         const GeoProjection* gp) const = 0;

  const QString& name() const {return m_name;}


  virtual ~ChartFileReader() = default;

  static QRectF computeBBox(S57::ElementDataVector &elems,
                            const GL::VertexVector& vertices,
                            const GL::IndexVector& indices);

  static QRectF computeSoundingsBBox(const GL::VertexVector& ps);

  static QPointF computeLineCenter(const S57::ElementDataVector &elems,
                                   const GL::VertexVector& vertices,
                                   const GL::IndexVector& indices);

  static QPointF computeAreaCenterAndBboxes(S57::ElementDataVector& elems,
                                            const GL::VertexVector& vertices,
                                            const GL::IndexVector& indices);

  static void triangulate(S57::ElementDataVector& elems,
                          GL::IndexVector& indices,
                          const GL::VertexVector& vertices,
                          const S57::ElementDataVector& edges);

  struct Edge {
    Edge() = default;
    Edge(const Edge& other) = default;
    Edge& operator =(const Edge& other) = default;
    quint32 begin;
    quint32 end;
    quint32 first;
    quint32 count;
    bool reversed;
    bool inner;
  };
  using EdgeVector = QVector<Edge>;
  using EdgeMap = QMap<quint32, Edge>;

  static S57::ElementDataVector createLineElements(GL::IndexVector& indices,
                                                   GL::VertexVector& vertices,
                                                   const EdgeVector& edges);

  static int addIndices(const Edge& e, GL::IndexVector& indices);


protected:

  ChartFileReader(const QString& name)
    : m_name(name) {}



  QString m_name;

};

class ChartFileReaderFactory {
public:

  ChartFileReader* loadReader() const;
  virtual QString name() const = 0;
  virtual QString displayName() const = 0;
  virtual QStringList filters() const = 0;

protected:

  virtual void initialize() const = 0;
  virtual ChartFileReader* create() const = 0;

  virtual ~ChartFileReaderFactory() = default;
};

Q_DECLARE_INTERFACE(ChartFileReaderFactory,
                    "net.kvanttiapina.qopencpn.ChartFileReaderFactory/1.0")
