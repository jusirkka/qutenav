/* -*- coding: utf-8-unix -*-
 *
 * File: oesencreader/src/oesencreader.cpp
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
