/* -*- coding: utf-8-unix -*-
 *
 * File: gshhsreader.cpp
 *
 * Copyright (C) 2022 Jukka Sirkka
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
#include "gshhsreader.h"
#include <QFileInfo>
#include "logging.h"
#include <climits>
#include <QDirIterator>
#include <QStandardPaths>
#include "shapereader.h"
#include "s52names.h"
#include <QDate>

WGS84Point GSHHSReader::index2Coord(quint32 index) {
  const quint32 D = index2Level(index);

  index &= ~Index_Mask;
  index >>= 2;

  const quint32 n0 = x0 / D;

  const int ix = index % n0;
  const int iy = index / n0;

  const qint32 xc = D * (2 * ix + 1) - x0;
  const qint32 yc = D * (2 * iy + 1) - y0;

  return WGS84Point::fromLL(.1 * xc, .1 * yc);
}

quint32 GSHHSReader::index2Level(quint32 index) {
  const quint32 levelBits = index & 0x03;
  const quint32 lvl = levelBits == 1 ? 3 : levelBits == 2 ? 10 : 30;
  return lvl;
}

quint32 GSHHSReader::coord2Index(const WGS84Point& p, quint32 D) {

  auto ix = static_cast<qint32>(std::floor((10. * p.lng() + x0) / 2 / D));
  ix = std::min(static_cast<qint32>(x0 / D - 1), std::max(0, ix));

  auto iy = static_cast<qint32>(std::floor((10 * p.lat() + y0) / 2 / D));
  iy = std::min(static_cast<qint32>(y0 / D - 1), std::max(0, iy));

  quint32 bits = iy * x0 / D + ix;
  bits <<= 2;

  const quint32 levelBits = D == 3 ? 1 : D == 10 ? 2 : 3;

  return Index_Mask | bits | levelBits;
}

const GeoProjection* GSHHSReader::geoprojection() const {
  return m_proj;
}

GSHHSReader::GSHHSReader(const QString& name)
  : ChartFileReader(name)
  , m_proj(GeoProjection::CreateProjection("SimpleMercator"))
{}

GSHHSReader::~GSHHSReader() {
  delete m_proj;
}


GeoProjection* GSHHSReader::configuredProjection(const QString& path) const {
  const auto ref = index2Coord(parsePath(path));
  auto gp = GeoProjection::CreateProjection(m_proj->className());
  gp->setReference(ref);

  return gp;
}


S57ChartOutline GSHHSReader::readOutline(const QString& path, const GeoProjection*) const {
  const auto ref = index2Coord(parsePath(path));
  const auto sw = WGS84Point::fromLL(ref.lng() - 1, ref.lat() - 1);
  const auto ne = WGS84Point::fromLL(ref.lng() + 1, ref.lat() + 1);
  return S57ChartOutline(sw, ne, WGS84Polygon(), WGS84Polygon(), UINT32_MAX, QDate(), QDate());
}

using GVector = ShapeReader::GeomAreaVector;
using GeomArea = ShapeReader::GeomArea;

namespace S57 {

// Helper class to set Object's private data
class ObjectBuilder {
public:
  void gshhsSetAttributes(S57::Object* obj, const AttributeMap& attrs) const {
    obj->m_attributes = attrs;
  }
  void gshhsSetGeometry(S57::Object* obj, S57::Geometry::Base* geom, const QRectF& bbox) const {
    obj->m_geometry = geom;
    obj->m_bbox = bbox;
  }
};

}

void GSHHSReader::readChart(GL::VertexVector& vertices,
                            GL::IndexVector& indices,
                            S57::ObjectVector& objects,
                            const QString& path,
                            const GeoProjection* gp) const {
  const auto index = parsePath(path);

  const auto ref = index2Coord(index);
  const auto lvl = index2Level(index);
  const double r = .1 * lvl;

  auto box = QRectF(gp->fromWGS84(WGS84Point::fromLL(ref.lng() - r, ref.lat() - r)),
                    gp->fromWGS84(WGS84Point::fromLL(ref.lng() + r, ref.lat() + r)));

  ShapeReader reader(box, gp);
  S57::ObjectBuilder helper;

  const quint32 newobj = S52::FindCIndex("NEWOBJ");
  const quint32 symins = S52::FindCIndex("SYMINS");
  const quint32 clsnam = S52::FindCIndex("CLSNAM");
  const quint32 clsdef = S52::FindCIndex("CLSDEF");

  auto obj = new S57::Object(1, newobj);
  auto geom = reader.createBoxGeometry(vertices, indices);
  helper.gshhsSetGeometry(obj, geom, box);
  S57::AttributeMap attrs;
  attrs[clsnam] = S57::Attribute(QString("GSHHSC"));
  attrs[symins] = S57::Attribute(QString("CS(GSHHSC01)"));
  attrs[clsdef] = S57::Attribute(QString("0"));
  helper.gshhsSetAttributes(obj, attrs);

  objects.append(obj);

  for (int i = 1; i <= 4; i++) {
    GVector geoms;
    reader.read(vertices, indices, geoms, GSHHG::ShapeFiles::instance()->path(i, lvl));
    qDebug() << "Appending" << geoms.size() << "objects of type" << i;
    quint32 index = i * 100000;
    for (const GeomArea& geom: geoms) {
      auto obj = new S57::Object(index++, newobj);
      helper.gshhsSetGeometry(obj, geom.area, geom.box);
      S57::AttributeMap attrs;
      attrs[clsnam] = S57::Attribute(QString("GSHHSC"));
      attrs[symins] = S57::Attribute(QString("CS(GSHHSC01)"));
      attrs[clsdef] = S57::Attribute(QString("%1").arg(i));
      helper.gshhsSetAttributes(obj, attrs);
      objects.append(obj);
    }
  }

}

quint32 GSHHSReader::parsePath(const QString &path) {
  auto parts = path.split(QLatin1Char('/'));
  if (parts.size() < 2 || parts[0] != "gshhs:") {
    throw ChartFileError(QString("%1 is not a proper GSHHS path").arg(path));
  }
  bool ok;
  const auto index = parts.last().toUInt(&ok);
  if (!ok) {
    throw ChartFileError(QString("%1 is not a proper GSHHS path").arg(path));
  }
  return index;
}


QString GSHHSReaderFactory::name() const {
  return "gshhs";
}

QString GSHHSReaderFactory::displayName() const {
  return "GSHHS Background";
}

QStringList GSHHSReaderFactory::filters() const {
  return QStringList(); // not used
}

void GSHHSReaderFactory::initialize(const QStringList& locs) const {
  GSHHG::ShapeFiles::instance()->init(locs);
}

ChartFileReader* GSHHSReaderFactory::create() const {
  return new GSHHSReader(name());
}

GSHHG::ShapeFiles* GSHHG::ShapeFiles::instance() {
  static ShapeFiles* s = new ShapeFiles;
  return s;
}

void GSHHG::ShapeFiles::init(const QStringList& subdirs) {
  const QMap<int, QChar> levels {{3, 'f'}, {10, 'h'}, {30, 'i'}};

  m_paths[3] = QStringList();
  m_paths[10] = QStringList();
  m_paths[30] = QStringList();

  for (auto it = m_paths.begin(); it != m_paths.end(); ++it) {
    it.value() << "" << "" << "" << "";
  }

  for (const QString& loc: QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
    for (auto lit = levels.cbegin(); lit != levels.cend(); ++lit) {
      for (int i = 0; i < 4; i++) {
        for (const QString& subdir: subdirs) {
          QDirIterator it(loc + QDir::separator() + subdir,
                          {QString("GSHHS_%1_L%2.shp").arg(lit.value()).arg(i + 1)},
                          QDir::Files | QDir::Readable,
                          QDirIterator::FollowSymlinks | QDirIterator::Subdirectories);

          if (it.hasNext()) {
            QFileInfo info(it.next());
            m_paths[lit.key()][i] = info.absoluteFilePath();
          }
        }
      }
    }
  }
  // qDebug() << m_paths;
}


QString GSHHG::ShapeFiles::path(int index, int level) const {
  return m_paths[level][index - 1];
}
