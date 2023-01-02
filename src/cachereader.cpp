/* -*- coding: utf-8-unix -*-
 *
 * File: src/cachereader.cpp
 *
 * Copyright (C) 2021 Jukka Sirkka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "cachereader.h"
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QFile>
#include <QDate>
#include <QDataStream>
#include "platform.h"

const GeoProjection* CacheReader::geoprojection() const {
  return m_proj;
}

CacheReader::CacheReader()
  : ChartFileReader("cache")
  , m_proj(GeoProjection::CreateProjection("SimpleMercator"))
{}

CacheReader::~CacheReader() {
  delete m_proj;
}

GeoProjection* CacheReader::configuredProjection(const QString &path) const {
  auto id = CacheId(path);
  const auto base = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);

  const auto cachePath = QString("%1/%2/%3").arg(base).arg(Platform::base_app_name()).arg(QString(id));

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

  const auto cachePath = QString("%1/%2/%3").arg(base).arg(Platform::base_app_name()).arg(QString(id));

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
  double lat;
  stream >> lng >> lat;
  const auto ref = WGS84Point::fromLL(lng , lat);

  // extent
  //  stream >> lng >> lat;
  //  const auto sw = WGS84Point::fromLL(lng , lat);
  //  stream >> lng >> lat;
  //  const auto ne = WGS84Point::fromLL(lng , lat);

  // cov/nocov
  //  auto decodeCov = [&stream, &lng, &lat] (WGS84Polygon& tgt) {
  //    int Ncov;
  //    stream >> Ncov;
  //    for (int nc = 0; nc < Ncov; nc++) {
  //      int Npol;
  //      stream >> Npol;
  //      WGS84PointVector pol;
  //      for (int n = 0; n < Npol; n++) {
  //        stream >> lng >> lat;
  //        pol << WGS84Point::fromLL(lng , lat);
  //      }
  //      tgt.append(pol);
  //    }
  //  };

  //  WGS84Polygon cov;
  //  decodeCov(cov);
  //  WGS84Polygon nocov;
  //  decodeCov(nocov);

  file.close();

  // Only reference point needed
  return S57ChartOutline(WGS84Point(),
                         WGS84Point(),
                         WGS84Polygon(),
                         WGS84Polygon(),
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

  const auto cachePath = QString("%1/%2/%3").arg(base).arg(Platform::base_app_name()).arg(QString(id));

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
  stream >> dummy >> dummy; // ref
  // extent
  //  stream >> dummy >> dummy;
  //  stream >> dummy >> dummy;

  // cov & nocov
  //  auto decodeCov = [&stream, &dummy] () {
  //    int Ncov;
  //    stream >> Ncov;
  //    for (int nc = 0; nc < Ncov; nc++) {
  //      int Npol;
  //      stream >> Npol;
  //      for (int n = 0; n < Npol; n++) {
  //        stream >> dummy >> dummy;
  //      }
  //    }
  //  };
  //  decodeCov();
  //  decodeCov();

  stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

  int Nc;
  GLfloat v;

  // vertices
  stream >> Nc;

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

  if (file.open(QFile::ReadWrite)) { // update mtime - saner cache mgmt
    QDataStream stream2(&file);
    stream2.setVersion(QDataStream::Qt_5_6);
    stream2.setByteOrder(QDataStream::LittleEndian);
    // rewrite magic
    stream2.writeRawData(id.constData(), 8);
    file.close();
  }

}

QByteArray CacheReader::CacheId(const QString& path) {
  QCryptographicHash hash(QCryptographicHash::Sha1);
  hash.addData(path.toUtf8());
  // convert sha1 to base36 form and return first 8 bytes for use as string
  return QByteArray::number(*reinterpret_cast<const quint64*>(hash.result().constData()), 36).left(8);
}
