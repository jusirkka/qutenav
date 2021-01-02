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

  static const int coord_section_offset = 10;
  static const int size_section_offset = 74;
  static const int payload_offset = 138;

  static const quint8 RelatedBit1 = 1;
  static const quint8 RelatedBit2 = 2;
  static const quint8 AttributeBit = 8;

  static const quint8 BorderBit = 1;
  static const quint8 InnerRingBit = 2;
  static const quint8 ReversedBit = 4;

  enum class EP {First, Last};

  struct Edge {
    int offset;
    int count;
    bool reversed;
    bool inner;
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
                          bool forcePolygons) const;

  void triangulate(S57::ElementDataVector& elems,
                   GL::IndexVector& indices,
                   const GL::VertexVector& vertices,
                   const S57::ElementDataVector& edges) const;

  QPointF computeAreaCenter(const S57::ElementDataVector &elems,
                            const GL::VertexVector& vertices,
                            const GL::IndexVector& indices) const;

  const int m_m_sor;
  const int m_wgsox;
  const int m_wgsoy;
  const int m_cscale;
  const int m_recdat;

  QMap<QString, quint32> m_subst;
  QMap<QString, QPair<quint32, S57::Attribute>> m_subst_attrs;

  QString m_name;
  GeoProjection* m_proj;

};



