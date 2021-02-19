#pragma once
#include "chartfilereader.h"

class CM93ReaderFactory;

class CM93Reader: public ChartFileReader {

  friend class CM93ReaderFactory;

public:

  const GeoProjection* geoprojection() const override;
  GeoProjection* configuredProjection(const QString &path) const override;

  S57ChartOutline readOutline(const QString& path, const GeoProjection* proj) const override;

  void readChart(GL::VertexVector& vertices,
                 GL::IndexVector& indices,
                 S57::ObjectVector& objects,
                 const QString& path,
                 const GeoProjection* proj) const override;


  using PointVector = QVector<QPointF>;
  using PRegion = QVector<PointVector>;
  using Region = S57ChartOutline::Region;


private:

  CM93Reader(const QString& name);

  static const int coord_section_offset = 10;
  static const int size_section_offset = 74;
  static const int payload_offset = 138;

  static const quint8 RelatedBit1 = 1;
  static const quint8 RelatedBit2 = 2;
  static const quint8 AttributeBit = 8;

  static const quint8 BorderBit = 1;
  static const quint8 InnerRingBit = 2;
  static const quint8 ReversedBit = 4;

  static const quint16 IndexMask = 0x1fff;
  static const uint IndexBits = 13;

  enum class EP {First, Last};

  struct Edge {
    int offset;
    int count;
    bool reversed;
    bool inner;
    bool border;
  };
  using EdgeVector = QVector<Edge>;


  QPointF getEndPoint(EP ep,
                      const Edge& e,
                      const GL::VertexVector& vertices) const;
  QPointF getPoint(int index,
                   const GL::VertexVector& vertices) const;

  int getEndPointIndex(EP ep, const Edge& e) const;
  int addAdjacent(int ep, int nbor, GL::VertexVector& vertices) const;
  int addIndices(const Edge& e, GL::IndexVector& indices) const;


  void createLineElements(S57::ElementDataVector& elems,
                          GL::IndexVector& indices,
                          GL::VertexVector& vertices,
                          const EdgeVector& edges,
                          bool triangles) const;

  PointVector addVertices(const Edge& e, const GL::VertexVector& vertices) const;

  PRegion createCoverage(const GL::VertexVector& vertices,
                         const EdgeVector& edges) const;

  Region transformCoverage(PRegion pcov, WGS84Point& sw, WGS84Point& ne,
                           const GeoProjection* gp) const;

  static const inline QMap<QString, quint32> scales = {{"Z", 20000000},
                                                       {"A",  3000000},
                                                       {"B",  1000000},
                                                       {"C",   200000},
                                                       {"D",   100000},
                                                       {"E",    50000},
                                                       {"F",    20000},
                                                       {"G",     7500}};


  const int m_m_sor;
  const int m_wgsox;
  const int m_wgsoy;
  const int m_recdat;

  const QMap<QString, quint32> m_subst;
  const QMap<QString, QPair<quint32, S57::Attribute>> m_subst_attrs;

  GeoProjection* m_proj;

};

class CM93ReaderFactory: public QObject, public ChartFileReaderFactory {

  Q_OBJECT
  Q_PLUGIN_METADATA(IID "net.kvanttiapina.qopencpn.ChartFileReaderFactory/1.0")
  Q_INTERFACES(ChartFileReaderFactory)

public:

  QString name() const override;
  QString displayName() const override;
  QStringList filters() const override;

protected:

  void initialize() const override;
  ChartFileReader* create() const override;

};




