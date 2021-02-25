#include "oesencreader.h"
#include "osenc.h"
#include "oedevice.h"



const GeoProjection* OesencReader::geoprojection() const {
  return m_proj;
}

OesencReader::OesencReader(const QString& name)
  : ChartFileReader(name)
  , m_proj(GeoProjection::CreateProjection("SimpleMercator"))
{}


GeoProjection* OesencReader::configuredProjection(const QString &path) const {

  OeDevice device(path, OeDevice::ReadHeader);
  if (!device.open(OeDevice::ReadOnly)) {
    throw ChartFileError(QString("Cannot open %1 for reading").arg(path));
  }

  Osenc senc;
  return senc.configuredProjection(&device, m_proj->className());
}

S57ChartOutline OesencReader::readOutline(const QString &path, const GeoProjection *gp) const {

  OeDevice device(path, OeDevice::ReadHeader);
  if (!device.open(OeDevice::ReadOnly)) {
    throw ChartFileError(QString("Cannot open %1 for reading").arg(path));
  }

  Osenc senc;
  return senc.readOutline(&device, gp);

}


void OesencReader::readChart(GL::VertexVector& vertices,
                            GL::IndexVector& indices,
                            S57::ObjectVector& objects,
                            const QString& path,
                            const GeoProjection* gp) const {
  OeDevice device(path, OeDevice::ReadSENC);
  device.open(OeDevice::ReadOnly);

  Osenc senc;
  senc.readChart(vertices, indices, objects, &device, gp);
}


QString OesencReaderFactory::name() const {
  return "oesenc";
}

QString OesencReaderFactory::displayName() const {
  return "OSENC Encrypted Charts";
}

QStringList OesencReaderFactory::filters() const {
  return QStringList {"*.oesenc"};
}

void OesencReaderFactory::initialize() const {
  OeDevice::Kickoff();
}

ChartFileReader* OesencReaderFactory::create() const {
  return new OesencReader(name());
}
