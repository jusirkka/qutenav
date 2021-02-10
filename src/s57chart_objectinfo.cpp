/* -*- coding: utf-8-unix -*-
 *
 * s57chart_objectinfo.cpp
 *
 * Created: 09/02/2021 2021 by Jukka Sirkka
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

#include "s57chart.h"
#include "s52names.h"
#include "utils.h"

static bool contains(const QRectF& box, const S57::Geometry::Point* ps, int& index) {
  if (ps->points().size() < 3) return box.contains(ps->center());
  const int N = ps->points().size() / 3;
  const S57::Geometry::PointVector ss = ps->points();
  for (int k = 0; k < N; k++) {
    const QPointF p(ss[3 * k], ss[3 * k + 1]);
    if (box.contains(p)) {
      index = 3 * k;
      return true;
    }
  }
  return false;
}


static bool crosses(const S57::Geometry::Line* ls, const glm::vec2* vertices,
                    const GLuint* indices, const QRectF& box) {
  const S57::ElementDataVector elems = ls->lineElements();
  for (const S57::ElementData& elem: elems) {
    if (!elem.bbox.intersects(box)) continue;
    const int n = elem.count - 2;
    const int first = elem.offset / sizeof(GLuint) + 1;
    for (int i0 = 0; i0 < n - 1; i0++) {
      const glm::vec2 p1 = vertices[indices[first + i0]];
      const glm::vec2 p2 = vertices[indices[first + i0 + 1]];
      if (!outsideBox(p1, p2, box)) return true;
    }
  }
  return false;
}

static bool includes(const S57::Geometry::Area* as, const glm::vec2* vs,
                     const GLuint* is, const QPointF& p) {

  auto inbox = [] (const S57::ElementData& elem, const QPointF& p) {
    return elem.bbox.contains(p);
  };

  auto closed = [is] (const S57::ElementData& elem) {
    auto first = elem.offset / sizeof(GLuint);
    auto last = first + elem.count - 1;
    // Note: adjacency
    return is[first + 1] == is[last - 1];
  };

  const S57::ElementDataVector elems = as->lineElements();
  if (!closed(elems.first())) return false;
  if (!inbox(elems.first(), p)) return false;
  if (!insidePolygon(elems.first().count, elems.first().offset, vs, is, p)) return false;

  for (int i = 1; i < elems.count(); i++) {
    if (!closed(elems[i])) continue;
    if (!inbox(elems[i], p)) continue;
    if (insidePolygon(elems[i].count, elems[i].offset, vs, is, p)) return false;
  }
  return true;
}


static S57::Description description(const S57::Object* obj) {
  S57::Description desc(S52::GetClassDescription(obj->classCode()));
  if (obj->geometry()->type() == S57::Geometry::Type::Point) {
    desc.attributes.append(S57::Pair("Location", obj->geometry()->centerLL().toISO6709()));
  }
  const S57::AttributeMap attributes = obj->attributes();
  for (S57::AttributeMap::const_iterator it = attributes.cbegin(); it != attributes.cend(); ++it) {
    desc.attributes.append(S57::Pair(S52::GetAttributeDescription(it.key()),
                                     S52::GetAttributeValueDescription(it.key(), it.value().value())));
  }
  return desc;
}

static S57::Description soundingDesc(const S57::Object* obj,
                                     const WGS84Point& p, qreal depth) {

  S57::Description desc(S52::GetClassDescription(obj->classCode()));

  desc.attributes.append(S57::Pair("Location", p.toISO6709()));
  desc.attributes.append(S57::Pair("Value", QString::number(depth)));

  const S57::AttributeMap attributes = obj->attributes();
  for (S57::AttributeMap::const_iterator it = attributes.cbegin(); it != attributes.cend(); ++it) {
    desc.attributes.append(S57::Pair(S52::GetAttributeDescription(it.key()),
                                     S52::GetAttributeValueDescription(it.key(), it.value().value())));
  }

  return desc;
}


S57::InfoType S57Chart::objectInfo(const WGS84Point& p, quint32 scale) {
  const auto q = m_nativeProj->fromWGS84(p);

  const QRectF va(m_nativeProj->fromWGS84(m_extent.sw()),
                  m_nativeProj->fromWGS84(m_extent.ne()));
  if (!va.contains(q)) return S57::InfoType();

  // 20 pixel resolution mapped to meters
  const float res = 20. / dots_per_mm_y * 0.001 * scale;
  const QRectF box(q - .5 * QPointF(res, res), QSizeF(res, res));

  QSet<quint32> handled;
  const quint32 c_lights = S52::FindIndex("LIGHTS");

  m_coordBuffer.bind();
  auto vertices = reinterpret_cast<const glm::vec2*>(m_coordBuffer.mapRange(0, m_staticVertexOffset, QOpenGLBuffer::RangeRead));

  m_indexBuffer.bind();
  auto indices = reinterpret_cast<const GLuint*>(m_indexBuffer.mapRange(0, m_staticElemOffset, QOpenGLBuffer::RangeRead));

  struct WrappedDesc {
    int geom;
    int prio;
    S57::Description desc;
  };

  const QMap<S57::Geometry::Type, int> tmap {{S57::Geometry::Type::Point, 1},
                                             {S57::Geometry::Type::Line, 2},
                                             {S57::Geometry::Type::Area, 3},
                                             {S57::Geometry::Type::Meta, 4}};
  QVector<WrappedDesc> wrapper;

  for (const ObjectLookup& p: m_lookups) {

    if (handled.contains(p.object->classCode())) continue;
    if (S52::IsMetaClass(p.object->classCode())) continue;
    if (!p.object->boundingBox().intersects(box)) continue;

    auto geom = p.object->geometry();

    WrappedDesc desc;
    desc.geom = tmap[geom->type()];
    desc.prio = p.lookup->priority();
    desc.desc.name = "";

    if (geom->type() == S57::Geometry::Type::Point) {
      auto ps = dynamic_cast<const S57::Geometry::Point*>(geom);
      int i = -1;
      if (contains(box, ps, i)) {
        if (i >= 0) {
          auto s = m_nativeProj->toWGS84(QPointF(ps->points()[i],
                                                 ps->points()[i + 1]));
          desc.desc = soundingDesc(p.object, s, ps->points()[i + 2]);
        } else {
          desc.desc = description(p.object);
        }
      }
    } else if (geom->type() == S57::Geometry::Type::Line) {
      auto ls = dynamic_cast<const S57::Geometry::Line*>(geom);
      if (crosses(ls, vertices, indices, box)) {
        desc.desc = description(p.object);
      }
    } else if (geom->type() == S57::Geometry::Type::Area) {
      auto as = dynamic_cast<const S57::Geometry::Area*>(geom);
      if (includes(as, vertices, indices, q)) {
        desc.desc = description(p.object);
      }
    }
    if (!desc.desc.name.isEmpty()) {
      if (p.object->classCode() != c_lights) {
        handled << p.object->classCode();
      }
      wrapper.append(desc);
    }
  }

  m_coordBuffer.unmap();
  m_coordBuffer.release();

  m_indexBuffer.unmap();
  m_indexBuffer.release();

  std::sort(wrapper.begin(), wrapper.end(), [] (const WrappedDesc& w1, const WrappedDesc& w2) {
    if (w1.geom != w2.geom) {
      return w1.geom < w2.geom;
    }
    return w1.prio > w2.prio;
  });

  S57::InfoType info;
  for (const WrappedDesc& desc: wrapper) {
    info.append(desc.desc);
  }

  return info;
}
