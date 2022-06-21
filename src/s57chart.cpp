/* -*- coding: utf-8-unix -*-
 *
 * File: src/s57chart.cpp
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
#include <functional>
#include "geoprojection.h"
#include "s57object.h"
#include "s52presentation.h"
#include "s52names.h"
#include <QDate>
#include "shader.h"
#include <QOpenGLExtraFunctions>
#include <QOpenGLContext>
#include "textmanager.h"
#include "rastersymbolmanager.h"
#include "vectorsymbolmanager.h"
#include "platform.h"
#include "chartfilereader.h"
#include "camera.h"
#include "chartmanager.h"
#include "geomutils.h"
#include "logging.h"

//
// S57Chart
//

namespace S57 {

// Helper class to set Object's private data
class ObjectBuilder {
public:
  void setOthers(S57::Object* obj,
                 Object::LocationHash* others,
                 Object::ContourVector* contours) const {
    obj->m_others = others;
    obj->m_contours = contours;
  }
  void addUnderling(S57::Object* obj,
                    S57::Object* underling) const {
    obj->m_underlings.append(underling);
  }
};

}

S57Chart::S57Chart(quint32 id, int prio, const QString& path)
  : QObject()
  , m_paintData(S52::Lookup::PriorityCount)
  , m_id(id)
  , m_priority(prio)
  , m_path(path)
  , m_infoSkipList {S52::FindCIndex("MAGVAR"),
                    S52::FindCIndex("ADMARE"),
                    S52::FindCIndex("CTNARE")}
  , m_navaids({S52::FindIndex("LITFLT"),
              S52::FindIndex("LITVES"),
              S52::FindIndex("BOYCAR"),
              S52::FindIndex("BOYINB"),
              S52::FindIndex("BOYISD"),
              S52::FindIndex("BOYLAT"),
              S52::FindIndex("BOYSAW"),
              S52::FindIndex("BOYSPP"),
              S52::FindIndex("boylat"),
              S52::FindIndex("boywtw"),
              S52::FindIndex("BCNCAR"),
              S52::FindIndex("BCNISD"),
              S52::FindIndex("BCNLAT"),
              S52::FindIndex("BCNSAW"),
              S52::FindIndex("BCNSPP"),
              S52::FindIndex("bcnlat"),
              S52::FindIndex("bcnwtw"),
              })
    , m_light(S52::FindIndex("LIGHTS"))
  , m_mutex()
  , m_proxy(nullptr)
{

  ChartFileReader* reader = nullptr;

  for (ChartFileReader* candidate: ChartManager::instance()->readers()) {
    try {
      m_nativeProj = candidate->configuredProjection(path);
    } catch (ChartFileError& e) {
      qCDebug(CS57) << candidate->name() << e.msg();
      continue;
    }
    reader = candidate;
    break;
  }


  if (reader == nullptr) {
    throw ChartFileError(QString("%1 is not a supported chart file").arg(path));
  }

  S57::ObjectVector objects;
  GL::VertexVector vertices;
  GL::IndexVector indices;

  reader->readChart(vertices, indices, objects, path, m_nativeProj);
  // Assume scaling has been applied in reader->readChart
  m_nativeProj->setScaling(QSizeF(1., 1.));

  S57::ObjectBuilder builder;
  QSet<double> contours;
  const quint32 depcnt = S52::FindIndex("DEPCNT");
  const quint32 valdco = S52::FindIndex("VALDCO");
  const quint32 depare = S52::FindIndex("DEPARE");
  const quint32 drgare = S52::FindIndex("DRGARE");

  S57::ObjectVector underlings;
  S57::ObjectVector overlings;

  for (S57::Object* object: objects) {
    // bind objects to lookup records
    S52::Lookup* lp = S52::FindLookup(object);
    m_lookups.append(ObjectLookup(object, lp));

    builder.setOthers(object, &m_locations, &m_contours);
    // sort point objects by their locations
    const WGS84Point p = object->geometry()->centerLL();
    if (p.valid()) {
      m_locations.insert(p, object);
    }
    // add contour values - these are needed in CS(DEPCNT02)
    if (object->classCode() == depcnt && object->attributeValue(valdco).isValid()) {
      contours << object->attributeValue(valdco).toDouble();
    }

    // underlings / overlings - needed for wrecks/obstructions
    if (lp->needUnderling()) {
      overlings.append(object);
    } else if (object->classCode() == depare || object->classCode() == drgare) {
      underlings.append(object);
    }
  }
  // Iterator constructors not yet supported in Qt 5.6.3
  // ContourVector sorted(contours.begin(), contours.end());
  ContourVector sorted;
  for (auto c: contours) sorted << c;
  std::sort(sorted.begin(), sorted.end());
  m_contours.append(sorted);

  for (auto overling: overlings) {
    findUnderling(overling, underlings, vertices, indices);
  }

  m_proxy = new GL::ChartProxy(vertices, indices);
}

void S57Chart::updateLookups() {
  ObjectLookupVector lookups;
  // Note: Lookup::needUnderling is equal within same feature: no need to update
  // underlings/overlings
  for (const ObjectLookup& p: m_lookups) {
    lookups.append(ObjectLookup(p.object, S52::FindLookup(p.object)));
  }
  m_lookups = lookups;
}

void S57Chart::encode(QDataStream& stream) {
  stream.setFloatingPointPrecision(QDataStream::DoublePrecision);

  // header
  const auto ref = m_nativeProj->reference();
  stream << ref.lng() << ref.lat();

  // coordinates
  stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

  using GForm = std::function<glm::vec2 (const glm::vec2&)>;
  GForm gform;
  // TODO: only mercator transforms supported for now
  if (geoProjection()->className() == "CM93Mercator") {
    gform = [] (const glm::vec2& v) {
      return static_cast<GLfloat>(CM93Mercator::scale) * v;
    };
  } else {
    gform = [] (const glm::vec2& v) {
      return v;
    };
  }

  // vertices
  auto vertices = reinterpret_cast<const glm::vec2*>(m_proxy->m_staticVertices.constData());
  const int Nc = m_proxy->m_staticVertices.size() / 2;

  stream << Nc;

  for (int n = 0; n < Nc; n++) {
    const glm::vec2 v = gform(vertices[n]);
    stream << v.x << v.y;
  }

  // indices
  auto indices = reinterpret_cast<const GLuint*>(m_proxy->m_staticIndices.constData());
  const int Ni = m_proxy->m_staticIndices.size();
  stream << Ni;

  for (int n = 0; n < Ni; n++) {
    stream << indices[n];
  }

  // objects
  S57::Transform transform;
  // TODO: only mercator transforms supported for now
  if (geoProjection()->className() == "CM93Mercator") {
    transform = [] (const QPointF& v) {
      return static_cast<qreal>(CM93Mercator::scale) * v;
    };
  } else {
    transform = [] (const QPointF& v) {
      return v;
    };
  }

  const int No = m_lookups.size();
  stream << No;

  for (const ObjectLookup& lup: m_lookups) {
    lup.object->encode(stream, transform);
  }

}

S57Chart::~S57Chart() {

  emit destroyProxy(m_proxy);

  for (ObjectLookup& d: m_lookups) {
    delete d.object;
  }

  for (S57::PaintDataMap& d: m_paintData) {
    for (S57::PaintMutIterator it = d.begin(); it != d.end(); ++it) {
      delete it.value();
    }
  }
  delete m_nativeProj;
}

void S57Chart::updatePaintData(const WGS84PointVector& cs, quint32 scale) {

  lock();

  // clear old paint data
  for (S57::PaintDataMap& d: m_paintData) {
    for (S57::PaintMutIterator it = d.begin(); it != d.end(); ++it) {
      delete it.value();
    }
    d.clear();
  }
  GL::VertexVector vertices;
  GL::VertexVector pivots;
  GL::VertexVector transforms;
  GL::VertexVector textTransforms;

  const auto maxcat = static_cast<quint8>(Conf::MarinerParams::MaxCategory());
  const auto today = QDate::currentDate();
  const bool showMeta = Conf::MarinerParams::ShowMeta();
  const quint32 unknownClass = S52::FindIndex("######");

  KV::Region cover(cs, m_nativeProj);

  const qreal sf = scaleFactor(cover.boundingRect(), scale);
  // qCDebug(CS57) << "scale factor =" << sf;

  SymbolPriorityVector rastersymbols(S52::Lookup::PriorityCount);
  SymbolPriorityVector vectorsymbols(S52::Lookup::PriorityCount);

  using Handler = std::function<void (const S57::PaintMutIterator&, int)>;

  auto parseLocals = [] (S57::PaintData::Type t, S57::PaintDataMap& pd, int prio, Handler func) {
    S57::PaintMutIterator it = pd.find(t);
    while (it != pd.end() && it.key() == t) {
      func(it, prio);
      it = pd.erase(it);
    }
  };

  auto handleLine = [this, sf, &vertices] (const S57::PaintMutIterator& it, int prio) {
    auto p = dynamic_cast<S57::Globalizer*>(it.value());
    const auto off = vertices.size() * sizeof(GLfloat);
    auto pn = p->globalize(off, sf);
    vertices += p->vertices(sf);
    delete p;
    m_paintData[prio].insert(pn->type(), pn);
  };

  auto mergeSymbols = [sf, cover] (SymbolPriorityVector& symbols, S57::PaintMutIterator it, int prio) {
    auto s = dynamic_cast<S57::SymbolPaintDataBase*>(it.value());
    if (symbols[prio].contains(s->key())) {
      auto s0 = dynamic_cast<S57::SymbolPaintDataBase*>(symbols[prio][s->key()]);
      s0->merge(s, sf, cover);
      delete s;
    } else {
      s->merge(nullptr, sf, cover);
      symbols[prio][s->key()] = it.value();
    }
  };

  auto mergeVectorSymbols = [&vectorsymbols, &mergeSymbols] (S57::PaintMutIterator it, int prio) {
    mergeSymbols(vectorsymbols, it, prio);
  };
  auto mergeRasterSymbols = [&rastersymbols, &mergeSymbols] (S57::PaintMutIterator it, int prio) {
    mergeSymbols(rastersymbols, it, prio);
  };

  TextColorPriorityVector textInstances(S52::Lookup::PriorityCount);

  auto mergeText = [&textInstances] (S57::PaintMutIterator it, int prio) {
    auto t = dynamic_cast<S57::TextElemData*>(it.value());
    if (textInstances[prio].contains(t->color())) {
      auto t0 = dynamic_cast<S57::TextElemData*>(textInstances[prio][t->color()]);
      t0->merge(t);
      delete t;
    } else {
      textInstances[prio][t->color()] = it.value();
    }
  };


  PaintPriorityVector updates(S52::Lookup::PriorityCount);

  for (ObjectLookup& d: m_lookups) {

    // check bbox & scale
    if (!d.object->canPaint(cover, scale, today, d.lookup->canOverride())) continue;

    // check display category
    if (!d.lookup->canOverride() && as_numeric(d.lookup->category()) > maxcat) {
      // qCDebug(CS57) << "Skipping by category" << S52::GetClassInfo(d.object->classCode());
      continue;
    }

    // Meta-object filter
    if (S52::IsMetaClass(d.object->classCode())) {
      // Filter out unknown meta classes
      if (d.lookup->classCode() == unknownClass) {
        // qCDebug(CS57) << "Filtering out" << S52::GetClassInfo(d.object->classCode());
        continue;
      }
      if (!showMeta) {
        continue;
      }
    }

    S57::PaintDataMap pd = d.lookup->execute(d.object);

    int prio = d.lookup->priority();
    // check category overrides
    if (pd.contains(S57::PaintData::Type::Override)) {
      auto ovr = pd.find(S57::PaintData::Type::Override);

      auto p = dynamic_cast<const S57::OverrideData*>(ovr.value());
      if (p->override()) {
        prio = 8;
      } else if (!d.object->canPaint(scale)) {
        for (S57::PaintMutIterator it = pd.begin(); it != pd.end(); ++it) {
          delete it.value();
        }
        continue;
      }
      delete p;
      pd.erase(ovr);
    } else if (d.lookup->canOverride() && // check overriddable objects
               (as_numeric(d.lookup->category()) > maxcat || !d.object->canPaint(scale))) {
      for (S57::PaintMutIterator it = pd.begin(); it != pd.end(); ++it) {
        delete it.value();
      }
      continue;
    }

    // check priority changes
    if (pd.contains(S57::PaintData::Type::Priority)) {
      auto pr = pd.find(S57::PaintData::Type::Priority);
      auto p = dynamic_cast<const S57::PriorityData*>(pr.value());
      prio = p->priority();
      delete p;
      pd.erase(pr);
    }

    updates[prio] += pd;
  }

  for (int prio = 0; prio < S52::Lookup::PriorityCount; prio++) {
    auto pd = updates[prio];
    // handle the local arrays
    parseLocals(S57::PaintData::Type::LineLocal, pd, prio, handleLine);

    // merge symbols & patterns
    parseLocals(S57::PaintData::Type::RasterSymbols, pd, prio, mergeRasterSymbols);
    parseLocals(S57::PaintData::Type::RasterPatterns, pd, prio, mergeRasterSymbols);
    parseLocals(S57::PaintData::Type::VectorSymbols, pd, prio, mergeVectorSymbols);
    parseLocals(S57::PaintData::Type::LucentVectorSymbols, pd, prio, mergeVectorSymbols);
    parseLocals(S57::PaintData::Type::VectorPatterns, pd, prio, mergeVectorSymbols);
    parseLocals(S57::PaintData::Type::VectorLineStyles, pd, prio, mergeVectorSymbols);
    // merge text
    parseLocals(S57::PaintData::Type::TextElements, pd, prio, mergeText);

    m_paintData[prio] += pd;
  }

  // move merged symbols & patterns to paintdatamap
  auto updatePaintDatamap = [this] (const SymbolPriorityVector& syms, GL::VertexVector& data) {
    for (int i = 0; i < S52::Lookup::PriorityCount; i++) {
      SymbolMap merged = syms[i];
      for (SymbolMutIterator it = merged.begin(); it != merged.end(); ++it) {
        auto s = dynamic_cast<S57::SymbolPaintDataBase*>(it.value());
        s->getPivots(data);
        m_paintData[i].insert(it.value()->type(), it.value());
      }
    }
  };

  updatePaintDatamap(rastersymbols, pivots);
  updatePaintDatamap(vectorsymbols, transforms);

  // filter area & line elements
  auto filterElements = [this, cover] (S57::PaintData::Type t) {
    for (int i = 0; i < S52::Lookup::PriorityCount; i++) {
      S57::PaintMutIterator it = m_paintData[i].find(t);
      const S57::PaintMutIterator end = m_paintData[i].end();
      while (it != end && it.key() == t) {
        it.value()->filterElements(cover);
        ++it;
      }
    }
  };

  filterElements(S57::PaintData::Type::TriangleArrays);
  filterElements(S57::PaintData::Type::LucentTriangleArrays);
  filterElements(S57::PaintData::Type::TriangleElements);
  filterElements(S57::PaintData::Type::LucentTriangleElements);
  filterElements(S57::PaintData::Type::LineArrays);
  filterElements(S57::PaintData::Type::LucentLineArrays);
  filterElements(S57::PaintData::Type::LineElements);
  filterElements(S57::PaintData::Type::LucentLineElements);

  // move merged text to paintdatamap
  for (int i = 0; i < S52::Lookup::PriorityCount; i++) {
    TextColorMap merged = textInstances[i];
    for (TextColorMutIterator it = merged.begin(); it != merged.end(); ++it) {
      auto t = dynamic_cast<S57::TextElemData*>(it.value());
      t->getInstances(textTransforms);
      m_paintData[i].insert(it.value()->type(), it.value());
    }
  }

  // Symbolized line updates to the transform buffer. Store remaining segments.
  for (int prio = 0; prio < S52::Lookup::PriorityCount; prio++) {
    LineVertexHash lineSegments;
    const S57::PaintMutIterator end = m_paintData[prio].end();
    S57::PaintMutIterator elem = m_paintData[prio].find(S57::PaintData::Type::VectorLineStyles);
    while (elem != end && elem.key() == S57::PaintData::Type::VectorLineStyles) {
      auto d = dynamic_cast<S57::LineStylePaintData*>(elem.value());
      GL::VertexVector segments;
      d->createTransforms(transforms,
                          segments,
                          m_proxy->m_staticVertices,
                          m_proxy->m_staticIndices);
      lineSegments[d->key()].append(segments);
      ++elem;
    }
    // update vertices & paint data from lineSegments
    for (LineVertexIterator it = lineSegments.cbegin(); it != lineSegments.cend(); ++it) {
      const auto off = vertices.size() * sizeof(GLfloat);
      const LineKey& key = it.key();
      m_paintData[prio].insert(S57::PaintData::Type::SegmentArrays,
                               new S57::SegmentArrayData(it.value().size() / 2,
                                                         off,
                                                         key.color,
                                                         key.width,
                                                         as_numeric(key.type)));
      vertices += it.value();
    }
  }

  m_proxy->m_dynamicVertices = vertices;
  m_proxy->m_pivots = pivots;
  m_proxy->m_transforms = transforms;
  m_proxy->m_textTransforms = textTransforms;

  unlock();
}


qreal S57Chart::scaleFactor(const QRectF& va, quint32 scale) const {

  const WGS84Point sw = m_nativeProj->toWGS84(va.topLeft());
  const WGS84Point nw = m_nativeProj->toWGS84(va.bottomLeft());

  // ratio of display (mm) and chart (m) unit heights
  return 1000. * (nw - sw).meters() / scale / va.height();
}

void S57Chart::updateModelTransform(const Camera *cam) {
  // TODO this works only for mercator projections.
  // Do linear approximation in general case.
  m_modelMatrix.setToIdentity();
  const QPointF p = cam->geoprojection()->fromWGS84(geoProjection()->reference());
  m_modelMatrix.translate(p.x(), p.y());
  if (geoProjection()->className() == "CM93Mercator") {
    m_modelMatrix.scale(CM93Mercator::scale, CM93Mercator::scale);
  }
}

void S57Chart::findUnderling(S57::Object *overling,
                              const S57::ObjectVector &candidates,
                              const GL::VertexVector &vertices,
                              const GL::IndexVector &indices) {
  const QPointF p = overling->geometry()->center();

  auto inbox = [] (const S57::ElementData& elem, const QPointF& p) {
    return elem.bbox.contains(p);
  };

  auto closed = [indices] (const S57::ElementData& elem) {
    auto first = elem.offset / sizeof(GLuint);
    auto last = first + elem.count - 1;
    // Note: adjacency
    return indices[first + 1] == indices[last - 1];
  };

  auto qs = reinterpret_cast<const glm::vec2*>(vertices.constData());
  auto is = indices.constData();

  for (S57::Object* c: candidates) {
    auto geom = dynamic_cast<const S57::Geometry::Line*>(c->geometry());
    const S57::ElementDataVector elems = geom->lineElements();
    if (!closed(elems.first())) continue;
    if (!inbox(elems.first(), p)) continue;
    if (!insidePolygon(elems.first().count, elems.first().offset, qs, is, p)) continue;
    bool isUnderling = true;
    for (int i = 1; i < elems.count(); i++) {
      if (!closed(elems[i])) continue;
      if (!inbox(elems[i], p)) continue;
      isUnderling = !insidePolygon(elems[i].count, elems[i].offset, qs, is, p);
      if (!isUnderling) break;
    }
    if (isUnderling) {
      S57::ObjectBuilder helper;
      helper.addUnderling(overling, c);
      return;
    }
  }
}

void S57Chart::drawAreas(const Camera* cam, int prio, bool blend) {

  const S57::PaintIterator end = m_paintData[prio].constEnd();

  const S57::PaintData::Type arrType = blend ? S57::PaintData::Type::LucentTriangleArrays :
                                               S57::PaintData::Type::TriangleArrays;
  const S57::PaintData::Type elemType = blend ? S57::PaintData::Type::LucentTriangleElements :
                                                S57::PaintData::Type::TriangleElements;

  S57::PaintIterator arr = m_paintData[prio].constFind(arrType);
  S57::PaintIterator elem = m_paintData[prio].constFind(elemType);

  if (arr == end && elem == end) return;

  auto prog = GL::AreaShader::instance();
  prog->initializePaint();

  m_proxy->m_staticCoordBuffer.bind();
  m_proxy->m_staticIndexBuffer.bind();

  prog->setGlobals(cam, m_modelMatrix);
  prog->setDepth(m_priority, prio);

  auto f = QOpenGLContext::currentContext()->extraFunctions();


  while (arr != end && arr.key() == arrType) {
    auto d = dynamic_cast<const S57::TriangleArrayData*>(arr.value());
    d->setUniforms();
    d->setVertexOffset();
    for (const S57::ElementData& e: d->elements()) {
      f->glDrawArrays(e.mode, e.offset, e.count);
    }
    ++arr;
  }


  while (elem != end && elem.key() == elemType) {
    auto d = dynamic_cast<const S57::TriangleElemData*>(elem.value());
    d->setUniforms();
    d->setVertexOffset();
    for (const S57::ElementData& e: d->elements()) {
      f->glDrawElements(e.mode, e.count, GL_UNSIGNED_INT,
                        reinterpret_cast<const void*>(e.offset));
    }
    ++elem;
  }


}


void S57Chart::drawLineArrays(const Camera* cam, int prio, bool blend) {

  const S57::PaintIterator end = m_paintData[prio].constEnd();

  const S57::PaintData::Type arrType = blend ? S57::PaintData::Type::LucentLineArrays :
                                               S57::PaintData::Type::LineArrays;

  S57::PaintIterator arr = m_paintData[prio].constFind(arrType);

  if (arr == end) return;

  auto prog = GL::LineArrayShader::instance();
  prog->initializePaint();

  prog->setGlobals(cam, m_modelMatrix);
  prog->setDepth(m_priority, prio);

  auto f = QOpenGLContext::currentContext()->extraFunctions();
  f->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_proxy->m_dynamicCoordBuffer.bufferId());

  while (arr != end && arr.key() == arrType) {
    auto d = dynamic_cast<const S57::LineArrayData*>(arr.value());
    d->setUniforms();
    for (const S57::ElementData& e: d->elements()) {
      d->setStorageOffsets(e.offset);
      f->glDrawArrays(GL_TRIANGLE_STRIP, 0, 2 * (e.count - 2));
    }
    ++arr;
  }
}

void S57Chart::drawSegmentArrays(const Camera* cam, int prio) {

  const S57::PaintIterator end = m_paintData[prio].constEnd();

  S57::PaintIterator arr = m_paintData[prio].constFind(S57::PaintData::Type::SegmentArrays);

  if (arr == end) return;

  auto prog = GL::SegmentArrayShader::instance();
  prog->initializePaint();

  prog->setGlobals(cam, m_modelMatrix);
  prog->setDepth(m_priority, prio);

  auto f = QOpenGLContext::currentContext()->extraFunctions();

  f->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_proxy->m_dynamicCoordBuffer.bufferId());

  while (arr != end && arr.key() == S57::PaintData::Type::SegmentArrays) {
    auto d = dynamic_cast<const S57::SegmentArrayData*>(arr.value());
    d->setUniforms();
    for (const S57::ElementData& e: d->elements()) {
      d->setStorageOffsets(e.offset);
      f->glDrawArrays(GL_TRIANGLES, 0, 3 * e.count);
    }
    ++arr;
  }
}

void S57Chart::drawLineElems(const Camera* cam, int prio, bool blend) {

  const S57::PaintIterator end = m_paintData[prio].constEnd();

  const S57::PaintData::Type elemType = blend ? S57::PaintData::Type::LucentLineElements :
                                                S57::PaintData::Type::LineElements;

  S57::PaintIterator elem = m_paintData[prio].constFind(elemType);

  if (elem == end) return;

  auto prog = GL::LineElemShader::instance();
  prog->initializePaint();

  prog->setGlobals(cam, m_modelMatrix);
  prog->setDepth(m_priority, prio);

  auto f = QOpenGLContext::currentContext()->extraFunctions();

  f->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_proxy->m_staticCoordBuffer.bufferId());
  f->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_proxy->m_staticIndexBuffer.bufferId());

  while (elem != end && elem.key() == elemType) {
    auto d = dynamic_cast<const S57::LineElemData*>(elem.value());
    d->setUniforms();
    for (const S57::ElementData& e: d->elements()) {
      d->setStorageOffsets(e.offset);
      f->glDrawArrays(GL_TRIANGLE_STRIP, 0, 2 * (e.count - 2));
    }
    ++elem;
  }
}

void S57Chart::drawText(const Camera* cam, int prio) {

  const S57::PaintIterator end = m_paintData[prio].constEnd();

  S57::PaintIterator elem = m_paintData[prio].constFind(S57::PaintData::Type::TextElements);

  if (elem == end) return;

  auto prog = GL::TextShader::instance();
  prog->initializePaint();

  TextManager::instance()->bind();

  prog->setGlobals(cam, m_modelMatrix);
  prog->setDepth(m_priority, prio);

  auto f = QOpenGLContext::currentContext()->extraFunctions();

  while (elem != end && elem.key() == S57::PaintData::Type::TextElements) {
    auto d = dynamic_cast<const S57::TextElemData*>(elem.value());
    d->setUniforms();
    m_proxy->m_textTransformBuffer.bind();
    d->setVertexOffset();
    f->glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, d->count());
    ++elem;
  }
}

void S57Chart::drawRasterSymbols(const Camera* cam, int prio) {

  const S57::PaintIterator end = m_paintData[prio].constEnd();

  S57::PaintIterator elem = m_paintData[prio].constFind(S57::PaintData::Type::RasterSymbols);

  if (elem == end) return;

  auto prog = GL::RasterSymbolShader::instance();
  prog->initializePaint();

  RasterSymbolManager::instance()->bind();

  prog->setGlobals(cam, m_modelMatrix);
  prog->setDepth(m_priority, prio);

  auto f = QOpenGLContext::currentContext()->extraFunctions();

  while (elem != end && elem.key() == S57::PaintData::Type::RasterSymbols) {
    auto d = dynamic_cast<const S57::RasterSymbolPaintData*>(elem.value());
    d->setUniforms();
    m_proxy->m_pivotBuffer.bind();
    d->setVertexOffset();
    auto e = d->element();
    f->glDrawElementsInstanced(e.mode,
                               e.count,
                               GL_UNSIGNED_INT,
                               reinterpret_cast<const void*>(e.offset),
                               d->count());
    ++elem;
  }
}


void S57Chart::drawVectorSymbols(const Camera* cam, int prio, bool blend) {

  const S57::PaintIterator end = m_paintData[prio].constEnd();

  const S57::PaintData::Type elemType = blend ? S57::PaintData::Type::LucentVectorSymbols :
                                                S57::PaintData::Type::VectorSymbols;

  S57::PaintIterator elem = m_paintData[prio].constFind(elemType);

  if (blend && elem == end) return;

  S57::PaintIterator lelem = m_paintData[prio].constFind(S57::PaintData::Type::VectorLineStyles);

  if (!blend && elem == end && lelem == end) return;

  auto prog = GL::VectorSymbolShader::instance();
  prog->initializePaint();

  VectorSymbolManager::instance()->bind();

  prog->setGlobals(cam, m_modelMatrix);
  prog->setDepth(m_priority, prio);

  auto f = QOpenGLContext::currentContext()->extraFunctions();

  while (elem != end && elem.key() == elemType) {
    auto d = dynamic_cast<const S57::VectorSymbolPaintData*>(elem.value());
    d->setUniforms();
    m_proxy->m_transformBuffer.bind();
    d->setVertexOffset();
    for (const S57::ColorElementData& e: d->elements()) {
      d->setColor(e.color);
      f->glDrawElementsInstanced(e.element.mode,
                                 e.element.count,
                                 GL_UNSIGNED_INT,
                                 reinterpret_cast<const void*>(e.element.offset),
                                 d->count());
    }
    ++elem;
  }

  if (blend) return;

  while (lelem != end && lelem.key() == S57::PaintData::Type::VectorLineStyles) {
    auto d = dynamic_cast<const S57::LineStylePaintData*>(lelem.value());
    d->setUniforms();
    m_proxy->m_transformBuffer.bind();
    d->setVertexOffset();
    for (const S57::ColorElementData& e: d->elements()) {
      d->setColor(e.color);
      f->glDrawElementsInstanced(e.element.mode,
                                 e.element.count,
                                 GL_UNSIGNED_INT,
                                 reinterpret_cast<const void*>(e.element.offset),
                                 d->count());
    }
    ++lelem;
  }
}

void S57Chart::drawRasterPatterns(const Camera *cam) {

  auto f = QOpenGLContext::currentContext()->extraFunctions();
  f->glEnable(GL_STENCIL_TEST);

  const auto t = S57::PaintData::Type::RasterPatterns;
  using Data = S57::PatternPaintData::AreaData;

  for (int prio = 0; prio < S52::Lookup::PriorityCount; prio++) {
    S57::PaintIterator end = m_paintData[prio].constEnd();
    S57::PaintIterator elem = m_paintData[prio].constFind(t);

    while (elem != end && elem.key() == t) {

      // stencil pattern areas
      m_proxy->m_staticCoordBuffer.bind();
      m_proxy->m_staticIndexBuffer.bind();
      GL::Shader* prog = GL::AreaShader::instance();
      prog->initializePaint();
      prog->setDepth(m_priority, prio);
      prog->setGlobals(cam, m_modelMatrix);

      f->glStencilFunc(GL_ALWAYS, 1, 0xff);
      f->glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
      f->glColorMask(false, false, false, false);
      f->glDepthMask(false);

      auto d = dynamic_cast<const S57::RasterPatternPaintData*>(elem.value());
      for (const Data& rd: d->areaArrays()) {
        d->setAreaVertexOffset(rd.vertexOffset);
        for (const S57::ElementData& e: rd.elements) {
          f->glDrawArrays(e.mode, e.offset, e.count);
        }
      }
      for (const Data& rd: d->areaElements()) {
        d->setAreaVertexOffset(rd.vertexOffset);
        for (const S57::ElementData& e: rd.elements) {
          f->glDrawElements(e.mode, e.count, GL_UNSIGNED_INT,
                            reinterpret_cast<const void*>(e.offset));
        }
      }

      f->glStencilFunc(GL_EQUAL, 1, 0xff);
      f->glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
      f->glDepthMask(true);
      f->glColorMask(true, true, true, true);

      RasterSymbolManager::instance()->bind();
      prog = GL::RasterSymbolShader::instance();
      prog->initializePaint();
      prog->setDepth(m_priority, prio);
      prog->setGlobals(cam, m_modelMatrix);

      d->setUniforms();
      m_proxy->m_pivotBuffer.bind();
      d->setVertexOffset();

      auto e = d->element();
      f->glDrawElementsInstanced(e.mode,
                                 e.count,
                                 GL_UNSIGNED_INT,
                                 reinterpret_cast<const void*>(e.offset),
                                 d->count());


      f->glClear(GL_STENCIL_BUFFER_BIT);

      ++elem;
    }
  }

  f->glDisable(GL_STENCIL_TEST);
}


void S57Chart::drawVectorPatterns(const Camera *cam) {

  auto f = QOpenGLContext::currentContext()->extraFunctions();
  f->glEnable(GL_STENCIL_TEST);

  const auto t = S57::PaintData::Type::VectorPatterns;
  using Data = S57::PatternPaintData::AreaData;

  for (int prio = 0; prio < S52::Lookup::PriorityCount; prio++) {
    S57::PaintIterator end = m_paintData[prio].constEnd();
    S57::PaintIterator elem = m_paintData[prio].constFind(t);

    while (elem != end && elem.key() == t) {

      // stencil pattern areas
      m_proxy->m_staticCoordBuffer.bind();
      m_proxy->m_staticIndexBuffer.bind();
      GL::Shader* prog = GL::AreaShader::instance();
      prog->initializePaint();
      prog->setDepth(m_priority, prio);
      prog->setGlobals(cam, m_modelMatrix);

      f->glStencilFunc(GL_ALWAYS, 1, 0xff);
      f->glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
      f->glDepthMask(false);
      f->glColorMask(false, false, false, false);

      auto d = dynamic_cast<const S57::VectorPatternPaintData*>(elem.value());
      for (const Data& rd: d->areaArrays()) {
        d->setAreaVertexOffset(rd.vertexOffset);
        for (const S57::ElementData& e: rd.elements) {
          f->glDrawArrays(e.mode, e.offset, e.count);
        }
      }
      for (const Data& rd: d->areaElements()) {
        d->setAreaVertexOffset(rd.vertexOffset);
        for (const S57::ElementData& e: rd.elements) {
          f->glDrawElements(e.mode, e.count, GL_UNSIGNED_INT,
                            reinterpret_cast<const void*>(e.offset));
        }
      }

      f->glStencilFunc(GL_EQUAL, 1, 0xff);
      f->glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
      f->glDepthMask(true);
      f->glColorMask(true, true, true, true);

      VectorSymbolManager::instance()->bind();
      prog = GL::VectorSymbolShader::instance();
      prog->initializePaint();
      prog->setDepth(m_priority, prio);
      prog->setGlobals(cam, m_modelMatrix);

      d->setUniforms();
      m_proxy->m_transformBuffer.bind();
      d->setVertexOffset();

      for (const S57::ColorElementData& e: d->elements()) {
        d->setColor(e.color);
        f->glDrawElementsInstanced(e.element.mode,
                                   e.element.count,
                                   GL_UNSIGNED_INT,
                                   reinterpret_cast<const void*>(e.element.offset),
                                   d->count());
      }

      f->glClear(GL_STENCIL_BUFFER_BIT);

      ++elem;
    }
  }

  f->glDisable(GL_STENCIL_TEST);
  f->glEnable(GL_DEPTH_TEST);
}

S57::InfoTypeFull S57Chart::objectInfoFull(const WGS84Point& p, quint32 scale) {
  const auto q = m_nativeProj->fromWGS84(p);

  // 20 pixel resolution mapped to meters
  const float res = 0.001 / dots_per_mm_y() * KV::PeepHoleSize * scale;
  const QRectF box(q - .5 * QPointF(res, res), QSizeF(res, res));

  QSet<quint32> handled;
  const quint32 c_lights = S52::FindIndex("LIGHTS");

  auto vertices = reinterpret_cast<const glm::vec2*>(m_proxy->m_staticVertices.constData());
  auto indices = reinterpret_cast<const GLuint*>(m_proxy->m_staticIndices.constData());

  struct WrappedDesc {
    int geom;
    int prio;
    S57::Description desc;
  };

  const QMap<S57::Geometry::Type, int> tmap {{S57::Geometry::Type::Point, 4},
                                             {S57::Geometry::Type::Line, 3},
                                             {S57::Geometry::Type::Area, 3},
                                             {S57::Geometry::Type::Meta, 1}};
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
      int soundingIndex = -1;
      if (ps->containedBy(box, soundingIndex)) {
        if (soundingIndex >= 0) {
          auto s = m_nativeProj->toWGS84(QPointF(ps->points()[soundingIndex],
                                                 ps->points()[soundingIndex + 1]));
          desc.desc = p.object->description(s, ps->points()[soundingIndex + 2]);
        } else {
          desc.desc = p.object->description();
        }
      }
    } else if (geom->type() == S57::Geometry::Type::Line) {
      auto ls = dynamic_cast<const S57::Geometry::Line*>(geom);
      if (ls->crosses(vertices, indices, box)) {
        desc.desc = p.object->description();
      }
    } else if (geom->type() == S57::Geometry::Type::Area) {
      auto as = dynamic_cast<const S57::Geometry::Area*>(geom);
      if (as->includes(vertices, indices, q)) {
        desc.desc = p.object->description();
      }
    }
    if (!desc.desc.name.isEmpty()) {
      if (p.object->classCode() != c_lights) {
        handled << p.object->classCode();
      }
      wrapper.append(desc);
    }
  }

  std::sort(wrapper.begin(), wrapper.end(), [] (const WrappedDesc& w1, const WrappedDesc& w2) {
    if (w1.geom != w2.geom) {
      return w1.geom > w2.geom;
    }
    return w1.prio > w2.prio;
  });

  S57::InfoTypeFull info;
  for (const WrappedDesc& desc: wrapper) {
    info.append(desc.desc);
  }

  return info;
}


S57::InfoType S57Chart::objectInfo(const WGS84Point& wp, quint32 scale) {
  const auto q = m_nativeProj->fromWGS84(wp);

  // Resolution in pixels mapped to meters
  const float res = 0.001 / dots_per_mm_y() * KV::PeepHoleSize * scale;
  const QRectF box(q - .5 * QPointF(res, res), QSizeF(res, res));

  auto vertices = reinterpret_cast<const glm::vec2*>(m_proxy->m_staticVertices.constData());
  auto indices = reinterpret_cast<const GLuint*>(m_proxy->m_staticIndices.constData());


  const QMap<S57::Geometry::Type, int> tmap {{S57::Geometry::Type::Point, 4},
                                             {S57::Geometry::Type::Line, 3},
                                             {S57::Geometry::Type::Area, 2},
                                             {S57::Geometry::Type::Meta, 1}};

  struct WrappedInfo {
    QString objectId;
    int priority;
    QStringList desc;
  };

  WrappedInfo info;
  info.priority = 0;
  qreal minArea = 1.e60;

  const auto maxcat = static_cast<quint8>(Conf::MarinerParams::MaxCategory());
  const auto today = QDate::currentDate();
  const KV::Region cover(box);


  for (int i = 0; i < m_lookups.size(); ++i) {

    const ObjectLookup& p = m_lookups[i];
    if (S52::IsMetaClass(p.object->classCode())) continue;
    if (m_infoSkipList.contains(p.object->classCode())) continue;

    // check bbox & scale
    if (!p.object->canPaint(cover, scale, today, p.lookup->canOverride())) continue;
    // check display category
    if (!p.lookup->canOverride() && as_numeric(p.lookup->category()) > maxcat) continue;

    auto geom = p.object->geometry();
    // select all points at same location; use highest priority for other geometries
    const int prio = 10 * tmap[geom->type()] +
        (geom->type() == S57::Geometry::Type::Point ? 0 : p.lookup->priority());
    if (prio < info.priority) continue;

    QString desc;
    if (geom->type() == S57::Geometry::Type::Point) {
      auto ps = dynamic_cast<const S57::Geometry::Point*>(geom);
      int soundingIndex = -1;
      if (ps->containedBy(box, soundingIndex)) {
        const QString depth = soundingIndex < 0 ? "" : QString("Sounding (%1m)").arg(ps->points()[soundingIndex + 2]);
        desc = p.lookup->description(p.object) + depth;
      }
    } else if (geom->type() == S57::Geometry::Type::Line) {
      auto ls = dynamic_cast<const S57::Geometry::Line*>(geom);
      if (ls->crosses(vertices, indices, box)) {
        desc = p.lookup->description(p.object);
      }
    } else if (geom->type() == S57::Geometry::Type::Area) {
      auto as = dynamic_cast<const S57::Geometry::Area*>(geom);
      if (as->includes(vertices, indices, q)) {
        desc = p.lookup->description(p.object);
      }
    }
    if (desc.isEmpty()) continue;

    if (info.priority == prio) {
      if (geom->type() == S57::Geometry::Type::Area) {
        const QRectF bbox = p.object->boundingBox();
        const qreal area = bbox.width() * bbox.height();
        if (area < minArea) {
          minArea = area;
          if (desc != "hidden") {
            info.desc.append(desc);
          }
          info.objectId = QString("%1/%2").arg(id()).arg(i);
          continue;
        }
      }
      if (desc != "hidden") {
        info.desc.append(desc);
      }
      info.objectId += QString("-%1").arg(i);
    } else {
      info.priority = prio;
      if (desc != "hidden") {
        info.desc.append(desc);
      }
      info.objectId = QString("%1/%2").arg(id()).arg(i);
    }
  }

  S57::InfoType ret;
  ret.objectId = info.objectId;
  ret.priority = info.priority;
  ret.info = info.desc.join("; ");
  return ret;
}

void S57Chart::paintIcon(QPainter& painter, quint32 objectIndex) const {
  if (static_cast<int>(objectIndex) >= m_lookups.size()) return;
  const ObjectLookup& p = m_lookups[objectIndex];
  p.lookup->paintIcon(painter, p.object);
}

void S57Chart::lock() {
  // qCDebug(CS57) << "locking";
  m_mutex.lock();
}

void S57Chart::unlock() {
  // qCDebug(CS57) << "unlocking";
  m_mutex.unlock();
}
