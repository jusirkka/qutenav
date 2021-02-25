#pragma once

#include "chartfilereader.h"

class OsencReaderFactory;

class OsencReader: public ChartFileReader {

  friend class OsencReaderFactory;

public:

  const GeoProjection* geoprojection() const override;
  GeoProjection* configuredProjection(const QString &path) const override;

  S57ChartOutline readOutline(const QString& path, const GeoProjection* proj) const override;

  void readChart(GL::VertexVector& vertices,
                 GL::IndexVector& indices,
                 S57::ObjectVector& objects,
                 const QString& path,
                 const GeoProjection* proj) const override;


private:

  OsencReader(const QString& name);

  GeoProjection* m_proj;

};

class OsencReaderFactory: public QObject, public ChartFileReaderFactory {

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

