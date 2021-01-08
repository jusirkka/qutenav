#include "s57chart.h"
#include <functional>
#include "geoprojection.h"
#include "s57object.h"
#include "s52presentation.h"
#include <QDate>
#include "settings.h"
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

S57Chart::S57Chart(quint32 id, const QString& path)
  : QObject()
  , m_paintData(S52::Lookup::PriorityCount)
  , m_updatedPaintData(S52::Lookup::PriorityCount)
  , m_id(id)
  , m_settings(Settings::instance())
  , m_coordBuffer(QOpenGLBuffer::VertexBuffer)
  , m_indexBuffer(QOpenGLBuffer::IndexBuffer)
  , m_pivotBuffer(QOpenGLBuffer::VertexBuffer)
  , m_transformBuffer(QOpenGLBuffer::VertexBuffer)
{

  ChartFileReader* reader = nullptr;
  S57ChartOutline outline;
  for (ChartFileReader* candidate: ChartManager::instance()->readers()) {
    try {
      outline = candidate->readOutline(path);
    } catch (ChartFileError& e) {
      qDebug() << e.msg();
      continue;
    }
    reader = candidate;
    break;
  }

  if (reader == nullptr) {
    throw ChartFileError(QString("%1 is not a supported chart file").arg(path));
  }

  m_nativeProj = GeoProjection::CreateProjection(reader->geoprojection()->className());
  m_nativeProj->setReference(outline.reference());
  m_nativeProj->setScaling(outline.scaling());

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
  ContourVector sorted(contours.begin(), contours.end());
  std::sort(sorted.begin(), sorted.end());
  m_contours.append(sorted);

  for (auto overling: overlings) {
    findUnderlings(overling, underlings, vertices, indices);
  }


  // fill in the buffers
  if (!m_coordBuffer.create()) qFatal("No can do");
  m_coordBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  m_coordBuffer.bind();
  m_staticVertexOffset = vertices.size() * sizeof(GLfloat);
  // add 10% extra space for vertices added later
  m_coordBuffer.allocate(m_staticVertexOffset + m_staticVertexOffset / 10);
  m_coordBuffer.write(0, vertices.constData(), m_staticVertexOffset);

  m_indexBuffer.create();
  m_indexBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  m_indexBuffer.bind();
  m_staticElemOffset = indices.size() * sizeof(GLuint);
  // add 10% extra space for elements added later
  m_indexBuffer.allocate(m_staticElemOffset + m_staticElemOffset / 10);
  m_indexBuffer.write(0, indices.constData(), m_staticElemOffset);

  m_pivotBuffer.create();
  m_pivotBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  m_pivotBuffer.bind();
  // 5K raster symbol/pattern instances
  m_pivotBuffer.allocate(5000 * 2 * sizeof(GLfloat));

  m_transformBuffer.create();
  m_transformBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  m_transformBuffer.bind();
  // 3K vector symbol/pattern instances
  m_transformBuffer.allocate(3000 * 4 * sizeof(GLfloat));

}

S57Chart::~S57Chart() {
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




void S57Chart::finalizePaintData() {
  // clear old paint data
  for (S57::PaintDataMap& d: m_paintData) {
    for (S57::PaintMutIterator it = d.begin(); it != d.end(); ++it) {
      delete it.value();
    }
  }

  m_paintData = m_updatedPaintData;

  // update vertex buffer
  m_coordBuffer.bind();
  GLsizei dataLen = m_staticVertexOffset + sizeof(GLfloat) * m_updatedVertices.size();
  if (dataLen > m_coordBuffer.size()) {
    auto staticData = new uchar[m_staticVertexOffset];
    auto coords = m_coordBuffer.map(QOpenGLBuffer::ReadOnly);
    memcpy(staticData, coords, m_staticVertexOffset);
    m_coordBuffer.unmap();
    m_coordBuffer.allocate(dataLen);
    m_coordBuffer.write(0, staticData, m_staticVertexOffset);
    delete [] staticData;
  }

  m_coordBuffer.write(m_staticVertexOffset, m_updatedVertices.constData(), dataLen - m_staticVertexOffset);

  // update pivot buffer
  m_pivotBuffer.bind();
  dataLen = sizeof(GLfloat) * m_updatedPivots.size();
  if (dataLen > m_pivotBuffer.size()) {
    m_pivotBuffer.allocate(dataLen);
  }

  m_pivotBuffer.write(0, m_updatedPivots.constData(), dataLen);

  // LC updates to the transform buffer
  for (int prio = 0; prio < S52::Lookup::PriorityCount; prio++) {
    const S57::PaintMutIterator end = m_paintData[prio].end();
    S57::PaintMutIterator elem = m_paintData[prio].find(S57::PaintData::Type::VectorLineStyles);
    while (elem != end && elem.key() == S57::PaintData::Type::VectorLineStyles) {
      auto d = dynamic_cast<S57::LineStylePaintData*>(elem.value());
      d->createTransforms(m_updatedTransforms,
                          m_coordBuffer,
                          m_indexBuffer,
                          m_staticVertexOffset);
      ++elem;
    }
  }

  // update transform buffer
  m_transformBuffer.bind();
  dataLen = sizeof(GLfloat) * m_updatedTransforms.size();
  if (dataLen > m_transformBuffer.size()) {
    m_transformBuffer.allocate(dataLen);
  }

  m_transformBuffer.write(0, m_updatedTransforms.constData(), dataLen);
}

void S57Chart::updatePaintData(const WGS84Point &sw, const WGS84Point &ne, quint32 scale) {

  // clear old updates
  for (S57::PaintDataMap& d: m_updatedPaintData) d.clear();
  m_updatedVertices.clear();
  m_updatedPivots.clear();
  m_updatedTransforms.clear();

  const auto maxcat = as_numeric(m_settings->maxCategory());
  const auto today = QDate::currentDate();
  const bool showMeta = m_settings->showMetaObjects();
  const quint32 qualClass = S52::FindIndex("M_QUAL");
  const quint32 unknownClass = S52::FindIndex("######");

  const QRectF viewArea(m_nativeProj->fromWGS84(sw),
                        m_nativeProj->fromWGS84(ne));

  const qreal sf = scaleFactor(sw, ne, viewArea, scale);

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

  auto handleLine = [this, sf] (const S57::PaintMutIterator& it, int prio) {
    auto p = dynamic_cast<S57::Globalizer*>(it.value());
    auto pn = p->globalize(m_staticVertexOffset + m_updatedVertices.size() * sizeof(GLfloat));
    m_updatedVertices += p->vertices(sf);
    delete p;
    m_updatedPaintData[prio].insert(pn->type(), pn);
  };

  auto mergeSymbols = [sf, viewArea] (SymbolPriorityVector& symbols, S57::PaintMutIterator it, int prio) {
    auto s = dynamic_cast<S57::SymbolPaintDataBase*>(it.value());
    if (symbols[prio].contains(s->key())) {
      auto s0 = dynamic_cast<S57::SymbolPaintDataBase*>(symbols[prio][s->key()]);
      s0->merge(s, sf, viewArea);
      delete s;
    } else {
      s->merge(nullptr, sf, viewArea);
      symbols[prio][s->key()] = it.value();
    }
  };

  auto mergeVectorSymbols = [&vectorsymbols, &mergeSymbols] (S57::PaintMutIterator it, int prio) {
    mergeSymbols(vectorsymbols, it, prio);
  };
  auto mergeRasterSymbols = [&rastersymbols, &mergeSymbols] (S57::PaintMutIterator it, int prio) {
    mergeSymbols(rastersymbols, it, prio);
  };

  PaintPriorityVector updates(S52::Lookup::PriorityCount);

  for (ObjectLookup& d: m_lookups) {
    // check bbox & scale
    if (!d.object->canPaint(viewArea, scale, today, d.lookup->canOverride())) continue;

    // check display category
    if (!d.lookup->canOverride() &&
        as_numeric(d.lookup->category()) > maxcat) {
      qDebug() << "Skipping by category" << S52::GetClassInfo(d.object->classCode());
      continue;
    }

    // Meta-object filter
    if (S52::IsMetaClass(d.object->classCode())) {
      // Filter out unknown meta classes
      if (d.lookup->classCode() == unknownClass) {
        // qDebug() << "Filtering out" << S52::GetClassInfo(d.object->classCode());
        continue;
      }
      if (!showMeta && d.object->classCode() != qualClass) {
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
    parseLocals(S57::PaintData::Type::SolidLineLocal, pd, prio, handleLine);
    parseLocals(S57::PaintData::Type::DashedLineLocal, pd, prio, handleLine);

    // merge symbols & patterns
    parseLocals(S57::PaintData::Type::RasterSymbols, pd, prio, mergeRasterSymbols);
    parseLocals(S57::PaintData::Type::RasterPatterns, pd, prio, mergeRasterSymbols);
    parseLocals(S57::PaintData::Type::VectorSymbols, pd, prio, mergeVectorSymbols);
    parseLocals(S57::PaintData::Type::VectorPatterns, pd, prio, mergeVectorSymbols);
    parseLocals(S57::PaintData::Type::VectorLineStyles, pd, prio, mergeVectorSymbols);

    m_updatedPaintData[prio] += pd;
  }

  // move merged symbols & patterns to paintdatamap
  auto updatePaintDatamap = [this] (const SymbolPriorityVector& syms, GL::VertexVector& data) {
    for (int i = 0; i < S52::Lookup::PriorityCount; i++) {
      SymbolMap merged = syms[i];
      for (SymbolMutIterator it = merged.begin(); it != merged.end(); ++it) {
        auto s = dynamic_cast<S57::SymbolPaintDataBase*>(it.value());
        s->getPivots(data);
        m_updatedPaintData[i].insert(it.value()->type(), it.value());
      }
    }
  };

  updatePaintDatamap(rastersymbols, m_updatedPivots);
  updatePaintDatamap(vectorsymbols, m_updatedTransforms);
}

qreal S57Chart::scaleFactor(const WGS84Point &sw, const WGS84Point &ne,
                            const QRectF& va, quint32 scale) const {
  const WGS84Point nw = WGS84Point::fromLL(sw.lng(), ne.lat());

  // ratio of screen pixel height and viewarea height
  return 1000. * (nw - sw).meters() / scale * dots_per_mm_y / va.height();
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

void S57Chart::findUnderlings(S57::Object *overling,
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

  // https://wrf.ecse.rpi.edu//Research/Short_Notes/pnpoly.html
  auto inpolygon = [vertices, indices] (const S57::ElementData& elem, const QPointF& p) {
    bool c = false;
    const int n = elem.count - 3;
    const int first = elem.offset / sizeof(GLuint) + 1;
    auto q = reinterpret_cast<const glm::vec2*>(vertices.constData());
    for (int i0 = 0, j0 = n - 1; i0 < n; j0 = i0++) {
      auto i = indices[first + i0];
      auto j = indices[first + j0];
      if (((q[i].y > p.y()) != (q[j].y > p.y())) &&
          (p.x() < (q[j].x - q[i].x) * (p.y() - q[i].y) / (q[j].y - q[i].y) + q[i].x)) {
        c = !c;
      }
    }
    return c;
  };

  for (S57::Object* c: candidates) {
    auto geom = dynamic_cast<const S57::Geometry::Line*>(c->geometry());
    const S57::ElementDataVector elems = geom->lineElements();
    if (!closed(elems.first())) continue;
    if (!inbox(elems.first(), p)) continue;
    if (!inpolygon(elems.first(), p)) continue;
    bool isUnderling = true;
    for (int i = 1; i < elems.count(); i++) {
      if (!closed(elems[i])) continue;
      if (!inbox(elems[i], p)) continue;
      isUnderling = !inpolygon(elems[i], p);
      if (!isUnderling) break;
    }
    if (isUnderling) {
      S57::ObjectBuilder helper;
      helper.addUnderling(overling, c);
      return;
    }
  }
}

void S57Chart::drawAreas(const Camera* cam, int prio) {

  m_coordBuffer.bind();
  m_indexBuffer.bind();

  auto prog = GL::AreaShader::instance();

  prog->setGlobals(cam, m_modelMatrix);
  prog->setDepth(prio);


  auto f = QOpenGLContext::currentContext()->extraFunctions();

  const S57::PaintIterator end = m_paintData[prio].constEnd();

  S57::PaintIterator arr = m_paintData[prio].constFind(S57::PaintData::Type::TriangleArrays);

  while (arr != end && arr.key() == S57::PaintData::Type::TriangleArrays) {
    auto d = dynamic_cast<const S57::TriangleArrayData*>(arr.value());
    d->setUniforms();
    d->setVertexOffset();
    for (const S57::ElementData& e: d->elements()) {
      f->glDrawArrays(e.mode, e.offset, e.count);
    }
    ++arr;
  }

  S57::PaintIterator elem = m_paintData[prio].constFind(S57::PaintData::Type::TriangleElements);

  while (elem != end && elem.key() == S57::PaintData::Type::TriangleElements) {
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

void S57Chart::drawSolidLines(const Camera* cam, int prio) {

  m_coordBuffer.bind();
  m_indexBuffer.bind();

  auto prog = GL::SolidLineShader::instance();

  prog->setGlobals(cam, m_modelMatrix);
  prog->setDepth(prio);


  auto f = QOpenGLContext::currentContext()->extraFunctions();

  const S57::PaintIterator end = m_paintData[prio].constEnd();

  S57::PaintIterator arr = m_paintData[prio].constFind(S57::PaintData::Type::SolidLineArrays);

  while (arr != end && arr.key() == S57::PaintData::Type::SolidLineArrays) {
    auto d = dynamic_cast<const S57::SolidLineArrayData*>(arr.value());
    d->setUniforms();
    d->setVertexOffset();
    for (const S57::ElementData& e: d->elements()) {
      f->glDrawArrays(e.mode, e.offset, e.count);
    }
    ++arr;
  }

  S57::PaintIterator elem = m_paintData[prio].constFind(S57::PaintData::Type::SolidLineElements);

  while (elem != end && elem.key() == S57::PaintData::Type::SolidLineElements) {
    auto d = dynamic_cast<const S57::SolidLineElemData*>(elem.value());
    d->setUniforms();
    d->setVertexOffset();
    for (const S57::ElementData& e: d->elements()) {
      f->glDrawElements(e.mode, e.count, GL_UNSIGNED_INT,
                        reinterpret_cast<const void*>(e.offset));
    }
    ++elem;
  }
}

void S57Chart::drawDashedLines(const Camera* cam, int prio) {

  m_coordBuffer.bind();
  m_indexBuffer.bind();

  auto prog = GL::DashedLineShader::instance();
  prog->setGlobals(cam, m_modelMatrix);
  prog->setDepth(prio);

  auto f = QOpenGLContext::currentContext()->extraFunctions();


  const S57::PaintIterator end = m_paintData[prio].constEnd();

  S57::PaintIterator arr = m_paintData[prio].constFind(S57::PaintData::Type::DashedLineArrays);

  while (arr != end && arr.key() == S57::PaintData::Type::DashedLineArrays) {
    auto d = dynamic_cast<const S57::DashedLineArrayData*>(arr.value());
    d->setUniforms();
    d->setVertexOffset();
    for (const S57::ElementData& e: d->elements()) {
      f->glDrawArrays(e.mode, e.offset, e.count);
    }
    ++arr;
  }

  S57::PaintIterator elem = m_paintData[prio].constFind(S57::PaintData::Type::DashedLineElements);

  while (elem != end && elem.key() == S57::PaintData::Type::DashedLineElements) {
    auto d = dynamic_cast<const S57::DashedLineElemData*>(elem.value());
    d->setUniforms();
    d->setVertexOffset();
    for (const S57::ElementData& e: d->elements()) {
      f->glDrawElements(e.mode, e.count, GL_UNSIGNED_INT,
                        reinterpret_cast<const void*>(e.offset));
    }
    ++elem;
  }
}


void S57Chart::drawText(const Camera* cam, int prio) {

  TextManager::instance()->bind();

  auto prog = GL::TextShader::instance();

  prog->setGlobals(cam, m_modelMatrix);
  prog->setDepth(prio);

  auto f = QOpenGLContext::currentContext()->extraFunctions();

  const S57::PaintIterator end = m_paintData[prio].constEnd();
  S57::PaintIterator elem = m_paintData[prio].constFind(S57::PaintData::Type::TextElements);

  while (elem != end && elem.key() == S57::PaintData::Type::TextElements) {
    auto d = dynamic_cast<const S57::TextElemData*>(elem.value());
    d->setUniforms();
    d->setVertexOffset();
    f->glDrawElements(d->elements().mode, d->elements().count, GL_UNSIGNED_INT,
                      reinterpret_cast<const void*>(d->elements().offset));
    ++elem;
  }
}

void S57Chart::drawRasterSymbols(const Camera* cam, int prio) {

  RasterSymbolManager::instance()->bind();

  auto prog = GL::RasterSymbolShader::instance();

  prog->setGlobals(cam, m_modelMatrix);
  prog->setDepth(prio);

  auto f = QOpenGLContext::currentContext()->extraFunctions();

  const S57::PaintIterator end = m_paintData[prio].constEnd();
  S57::PaintIterator elem = m_paintData[prio].constFind(S57::PaintData::Type::RasterSymbols);

  while (elem != end && elem.key() == S57::PaintData::Type::RasterSymbols) {
    auto d = dynamic_cast<const S57::RasterSymbolPaintData*>(elem.value());
    d->setUniforms();
    m_pivotBuffer.bind();
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


void S57Chart::drawVectorSymbols(const Camera* cam, int prio) {

  VectorSymbolManager::instance()->bind();

  auto prog = GL::VectorSymbolShader::instance();

  prog->setGlobals(cam, m_modelMatrix);
  prog->setDepth(prio);

  auto f = QOpenGLContext::currentContext()->extraFunctions();

  const S57::PaintIterator end = m_paintData[prio].constEnd();

  S57::PaintIterator elem = m_paintData[prio].constFind(S57::PaintData::Type::VectorSymbols);
  while (elem != end && elem.key() == S57::PaintData::Type::VectorSymbols) {
    auto d = dynamic_cast<const S57::VectorSymbolPaintData*>(elem.value());
    d->setUniforms();
    m_transformBuffer.bind();
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

  elem = m_paintData[prio].constFind(S57::PaintData::Type::VectorLineStyles);
  while (elem != end && elem.key() == S57::PaintData::Type::VectorLineStyles) {
    auto d = dynamic_cast<const S57::LineStylePaintData*>(elem.value());
    d->setUniforms();
    m_transformBuffer.bind();
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
}

void S57Chart::drawRasterPatterns(const Camera *cam) {

  auto f = QOpenGLContext::currentContext()->extraFunctions();
  f->glEnable(GL_STENCIL_TEST);
  f->glDisable(GL_DEPTH_TEST);

  const auto t = S57::PaintData::Type::RasterPatterns;
  using Data = S57::PatternPaintData::AreaData;

  for (int prio = 0; prio < S52::Lookup::PriorityCount; prio++) {
    S57::PaintIterator end = m_paintData[prio].constEnd();
    S57::PaintIterator elem = m_paintData[prio].constFind(t);

    while (elem != end && elem.key() == t) {

      // stencil pattern areas
      m_coordBuffer.bind();
      m_indexBuffer.bind();
      GL::Shader* prog = GL::AreaShader::instance();
      prog->initializePaint();
      prog->setDepth(prio);
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
      prog->setDepth(prio);
      prog->setGlobals(cam, m_modelMatrix);

      d->setUniforms();
      m_pivotBuffer.bind();
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

  f->glEnable(GL_DEPTH_TEST);
  f->glDisable(GL_STENCIL_TEST);
}


void S57Chart::drawVectorPatterns(const Camera *cam) {

  auto f = QOpenGLContext::currentContext()->extraFunctions();
  f->glEnable(GL_STENCIL_TEST);
  f->glDisable(GL_DEPTH_TEST);

  const auto t = S57::PaintData::Type::VectorPatterns;
  using Data = S57::PatternPaintData::AreaData;

  for (int prio = 0; prio < S52::Lookup::PriorityCount; prio++) {
    S57::PaintIterator end = m_paintData[prio].constEnd();
    S57::PaintIterator elem = m_paintData[prio].constFind(t);

    while (elem != end && elem.key() == t) {

      // stencil pattern areas
      m_coordBuffer.bind();
      m_indexBuffer.bind();
      GL::Shader* prog = GL::AreaShader::instance();
      prog->initializePaint();
      prog->setDepth(prio);
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
      prog->setDepth(prio);
      prog->setGlobals(cam, m_modelMatrix);

      d->setUniforms();
      m_transformBuffer.bind();
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

