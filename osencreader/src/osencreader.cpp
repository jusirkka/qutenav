/* -*- coding: utf-8-unix -*-
 *
 * File: osencreader/src/osencreader.cpp
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
#include "osencreader.h"
#include <QFile>
#include "osenc.h"



const GeoProjection* OsencReader::geoprojection() const {
  return m_proj;
}

OsencReader::OsencReader(const QString& name)
  : ChartFileReader(name)
  , m_proj(GeoProjection::CreateProjection("SimpleMercator"))
{}


GeoProjection* OsencReader::configuredProjection(const QString &path) const {

  QFile file(path);
  if (!file.open(QFile::ReadOnly)) {
    throw ChartFileError(QString("Cannot open %1 for reading").arg(path));
  }

  Osenc senc;
  return senc.configuredProjection(&file, m_proj->className());
}

S57ChartOutline OsencReader::readOutline(const QString &path, const GeoProjection *gp) const {

  QFile file(path);
  if (!file.open(QFile::ReadOnly)) {
    throw ChartFileError(QString("Cannot open %1 for reading").arg(path));
  }

  Osenc senc;
  return senc.readOutline(&file, gp);

}


void OsencReader::readChart(GL::VertexVector& vertices,
                            GL::IndexVector& indices,
                            S57::ObjectVector& objects,
                            const QString& path,
                            const GeoProjection* gp) const {
  QFile file(path);
  file.open(QFile::ReadOnly);

  Osenc senc;
  return senc.readChart(vertices, indices, objects, &file, gp);
}


QString OsencReaderFactory::name() const {
  return "osenc";
}

QString OsencReaderFactory::displayName() const {
  return "OSENC Charts";
}

QStringList OsencReaderFactory::filters() const {
  return QStringList {"*.S57"};
}

void OsencReaderFactory::initialize(const QStringList&) const {
  // noop
}

ChartFileReader* OsencReaderFactory::create() const {
  return new OsencReader(name());
}
