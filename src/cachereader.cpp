#include "cachereader.h"
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QFile>
#include <QDate>

const GeoProjection* CacheReader::geoprojection() const {
  return m_proj;
}

CacheReader::CacheReader()
  : ChartFileReader("cache")
  , m_proj(GeoProjection::CreateProjection("SimpleMercator"))
{}

GeoProjection* CacheReader::configuredProjection(const QString &path) const {
  auto id = CacheId(path);
  const auto base = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);

  const auto cachePath = QString("%1/%2/%3").arg(base).arg(qAppName()).arg(QString(id));

  QFile file(cachePath);
  if (!file.open(QFile::ReadOnly)) {
    throw ChartFileError(QString("Cannot open %1 for reading").arg(cachePath));
  }
  QDataStream stream(&file);

  stream.setVersion(QDataStream::Qt_5_6);
  stream.setByteOrder(QDataStream::LittleEndian);
  QByteArray magic(8, '0');
  stream.readRawData(magic.data(), 8);
  if (magic != id) {
    throw ChartFileError(QString("%1 is not a proper cached chart file").arg(cachePath));
  }

  stream.setFloatingPointPrecision(QDataStream::DoublePrecision);

  // header
  double lng;
  stream >> lng;
  double lat;
  stream >> lat;
  auto ref = WGS84Point::fromLL(lng , lat);

  file.close();

  auto gp = GeoProjection::CreateProjection(m_proj->className());
  gp->setReference(ref);
  return gp;

}


S57ChartOutline CacheReader::readOutline(const QString &path, const GeoProjection*) const {
  auto id = CacheId(path);
  const auto base = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);

  const auto cachePath = QString("%1/%2/%3").arg(base).arg(qAppName()).arg(QString(id));

  QFile file(cachePath);
  if (!file.open(QFile::ReadOnly)) {
    throw ChartFileError(QString("Cannot open %1 for reading").arg(cachePath));
  }
  QDataStream stream(&file);

  stream.setVersion(QDataStream::Qt_5_6);
  stream.setByteOrder(QDataStream::LittleEndian);
  QByteArray magic(8, '0');
  stream.readRawData(magic.data(), 8);
  if (magic != id) {
    throw ChartFileError(QString("%1 is not a proper cached chart file").arg(cachePath));
  }

  stream.setFloatingPointPrecision(QDataStream::DoublePrecision);

  // header
  double lng;
  stream >> lng;
  double lat;
  stream >> lat;
  auto ref = WGS84Point::fromLL(lng , lat);

  file.close();

  // Only reference point needed
  return S57ChartOutline(WGS84Point(),
                         WGS84Point(),
                         S57ChartOutline::Region(),
                         S57ChartOutline::Region(),
                         ref,
                         QSizeF(1., 1.),
                         1,
                         QDate(),
                         QDate());
}

void CacheReader::readChart(GL::VertexVector& vertices,
                            GL::IndexVector& indices,
                            S57::ObjectVector& objects,
                            const QString& path,
                            const GeoProjection*) const {

  auto id = CacheId(path);
  const auto base = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);

  const auto cachePath = QString("%1/%2/%3").arg(base).arg(qAppName()).arg(QString(id));

  QFile file(cachePath);
  if (!file.open(QFile::ReadOnly)) {
    throw ChartFileError(QString("Cannot open %1 for reading").arg(cachePath));
  }
  QDataStream stream(&file);

  stream.setVersion(QDataStream::Qt_5_6);
  stream.setByteOrder(QDataStream::LittleEndian);
  QByteArray magic(8, '0');
  stream.readRawData(magic.data(), 8);
  if (magic != id) {
    throw ChartFileError(QString("%1 is not a proper cached chart file").arg(cachePath));
  }

  stream.setFloatingPointPrecision(QDataStream::DoublePrecision);

  // header
  double dummy;
  stream >> dummy; // ref
  stream >> dummy;

  stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

  // vertices
  int Nc;
  stream >> Nc;

  GLfloat v;
  for (int n = 0; n < Nc; n++) {
    stream >> v;
    vertices << v;
    stream >> v;
    vertices << v;
  }

  // indices
  int Ni;
  stream >> Ni;

  GLuint k;
  for (int n = 0; n < Ni; n++) {
    stream >> k;
    indices << k;
  }

  // objects
  int No;
  stream >> No;

  for (int n = 0; n < No; n++) {
    objects.append(S57::Object::Decode(stream));
  }

  file.close();
}

QByteArray CacheReader::CacheId(const QString& path) {
  QCryptographicHash hash(QCryptographicHash::Sha1);
  hash.addData(path.toUtf8());
  // convert sha1 to base36 form and return first 8 bytes for use as string
  return QByteArray::number(*reinterpret_cast<const quint64*>(hash.result().constData()), 36).left(8);
}
