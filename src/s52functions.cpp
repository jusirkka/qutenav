#include "s52functions.h"
#include "s52presentation.h"
#include "conf_marinerparams.h"
#include "settings.h"
#include <cmath>
#include "types.h"
#include "textmanager.h"
#include "rastersymbolmanager.h"

S57::PaintDataMap S52::AreaColor::execute(const QVector<QVariant>& vals,
                                          const S57::Object* obj) {
  auto geom = dynamic_cast<const S57::Geometry::Area*>(obj->geometry());
  Q_ASSERT(geom != nullptr);

  S57::PaintData* p;

  auto color = S52::GetColor(vals[0].toUInt());
  color.setAlpha(vals[1].toUInt());

  if (geom->indexed()) {
    p = new S57::TriangleElemData(geom->triangleElements(), geom->vertexOffset(), color);
  } else {
    p = new S57::TriangleArrayData(geom->triangleElements(), geom->vertexOffset(), color);
  }

  return S57::PaintDataMap{{p->type(), p}};
}

S57::PaintDataMap S52::AreaPattern::execute(const QVector<QVariant>& vals,
                                            const S57::Object* obj) {

  const quint32 index = vals[0].toUInt();
  const SymbolData s = RasterSymbolManager::instance()->symbolData(index, S52::SymbolType::Pattern);

  if (!s.isValid()) {
    qWarning() << "Missing raster pattern. Vector patterns not implemented";
    return S57::PaintDataMap();
  }

  // qDebug() << "[Class]" << S52::GetClassInfo(obj->classCode());
  // qDebug() << "[Symbol]" << S52::GetSymbolInfo(index, S52::SymbolType::Pattern);
  // qDebug() << "[Location]" << obj->geometry()->centerLL().print();
  // for (auto k: obj->attributes().keys()) {
  //   qDebug() << GetAttributeInfo(k, obj);
  // }
  // qDebug() << "[Pattern]" << s.size() << s.advance().x << s.advance().xy;

  const QMetaType::Type t = static_cast<QMetaType::Type>(vals[1].type());
  Q_ASSERT(t == QMetaType::Float || t == QMetaType::Double);
  if (vals[1].toDouble() > 0.) {
    qWarning() << "Rotations not implemented" << vals[1].toDouble();
  }

  auto geom = dynamic_cast<const S57::Geometry::Area*>(obj->geometry());
  Q_ASSERT(geom != nullptr);

  S57::PaintData* p;

  if (geom->indexed()) {
    p = new S57::RasterPatternElemData(geom->triangleElements(),
                                       geom->vertexOffset(),
                                       obj->boundingBox(),
                                       s.offset(),
                                       s.advance(),
                                       s.elements(),
                                       index);
  } else {
    p = new S57::RasterPatternArrayData(geom->triangleElements(),
                                        geom->vertexOffset(),
                                        obj->boundingBox(),
                                        s.offset(),
                                        s.advance(),
                                        s.elements(),
                                        index);
  }

  return S57::PaintDataMap{{p->type(), p}};
}

S52::LineSimple::LineSimple(quint32 index)
  : Function("LS", index)
  , m_valnmr(S52::FindIndex("VALNMR"))
  , m_orient(S52::FindIndex("ORIENT")) {}

S57::PaintDataMap S52::LineSimple::execute(const QVector<QVariant>& vals,
                                           const S57::Object* obj) {
  auto line = dynamic_cast<const S57::Geometry::Line*>(obj->geometry());
  if (line == nullptr) {
    return linesFromPoint(vals, obj);
  }

  S57::PaintData* p;

  auto pattern = static_cast<S52::Lookup::Line>(vals[0].toUInt());
  auto width = vals[1].toUInt();
  auto color = S52::GetColor(vals[2].toUInt());

  if (pattern == S52::Lookup::Line::Solid) {
    p = new S57::SolidLineElemData(line->lineElements(), 0, color, width);
  } else {
    p = new S57::DashedLineElemData(line->lineElements(), 0, color, width,
                                    as_numeric(pattern));
  }

  return S57::PaintDataMap{{p->type(), p}};
}

S57::PaintDataMap S52::LineSimple::linesFromPoint(const QVector<QVariant> &vals, const S57::Object *obj) {
  auto point = dynamic_cast<const S57::Geometry::Point*>(obj->geometry());
  Q_ASSERT(point != nullptr);

  bool ok;
  const double orient = obj->attributeValue(m_orient).toDouble(&ok) * M_PI / 180.;
  if (!ok) {
    return S57::PaintDataMap();
  }

  double valnmr = obj->attributeValue(m_valnmr).toDouble(&ok);
  if (!ok) {
    valnmr = 9. * 1852.;
  }

  S57::ElementData e;
  e.count = 4;
  e.offset = 0;
  e.mode = GL_LINE_STRIP_ADJACENCY;

  S57::ElementDataVector elements;
  elements.append(e);

  S57::VertexVector vertices;
  const GLfloat x1 = point->points()[0];
  const GLfloat y1 = point->points()[1];
  const GLfloat x2 = x1 - valnmr * sin(orient);
  const GLfloat y2 = y1 - valnmr * cos(orient);

  vertices << 2 * x1 - x2 << 2 * y1 - y2;
  vertices << x1 << y1;
  vertices << x2 << y2;
  vertices << 2 * x2 - x1 << 2 * y2 - y1;


  S57::PaintData* p;

  auto pattern = static_cast<S52::Lookup::Line>(vals[0].toUInt());
  auto width = vals[1].toUInt();
  auto color = S52::GetColor(vals[2].toUInt());

  if (pattern == S52::Lookup::Line::Solid) {
    p = new S57::SolidLineLocalData(vertices, elements, color, width);
  } else {
    p = new S57::DashedLineLocalData(vertices, elements, color, width,
                                     as_numeric(pattern));
  }


  return S57::PaintDataMap{{p->type(), p}};
}


S57::PaintDataMap S52::LineComplex::execute(const QVector<QVariant>& /*vals*/,
                                            const S57::Object* /*obj*/) {
  qWarning() << "LC: not implemented";
  return S57::PaintDataMap(); // invalid paint data
}


S57::PaintDataMap S52::PointSymbol::execute(const QVector<QVariant>& vals,
                                            const S57::Object* obj) {

  const QMetaType::Type t = static_cast<QMetaType::Type>(vals[1].type());
  Q_ASSERT(t == QMetaType::Float || t == QMetaType::Double);
  if (vals[1].toDouble() > 0.) {
    qWarning() << "Rotations not implemented" << vals[1].toDouble();
  }

  quint32 index = vals[0].toUInt();
  const SymbolData s = RasterSymbolManager::instance()->symbolData(index, S52::SymbolType::Single);

  // qDebug() << "[Class]" << S52::GetClassInfo(obj->classCode());
  // qDebug() << "[Symbol]" << S52::GetSymbolInfo(index, S52::SymbolType::Single);
  // for (auto k: obj->attributes().keys()) {
  //   qDebug() << GetAttributeInfo(k, obj);
  // }

  if (!s.isValid()) {
    qDebug() << "Missing raster symbol. Vector symbols not implemented";
    return S57::PaintDataMap();
  }

  S57::PaintData* p = new S57::RasterSymbolElemData(obj->geometry()->center(),
                                                    s.offset(),
                                                    s.elements(),
                                                    index);

  return S57::PaintDataMap{{p->type(), p}};
}

S57::PaintDataMap S52::Text::execute(const QVector<QVariant>& vals,
                                     const S57::Object* obj) {

  const QString txt = vals[0].toString();
  if (txt.isEmpty()) {
    return S57::PaintDataMap();
  }
  // qDebug() << "TX:" << as_numeric(obj->geometry()->type()) << txt;

  const quint8 group = vals[8].toUInt();
  if (!Settings::instance()->textGrouping().contains(group)) {
    qWarning() << "skipping TX in group" << group;
    return S57::PaintDataMap();
  }

  const QString chars = vals[4].toString();

  Q_ASSERT(chars.left(1).toUInt() == 1); // style: always system's default
  const quint8 width = chars.mid(2, 1).toUInt();
  Q_ASSERT(width == 1 || width == 2); // always roman slant

  auto weight = as_enum<TXT::Weight>(chars.mid(1, 1).toUInt(), TXT::AllWeights);

  auto space = as_enum<TXT::Space>(vals[3].toUInt(), TXT::AllSpaces);
  if (space != TXT::Space::Standard/* && space != TXT::Space::Wrapped*/) {
    qWarning() << "TX: text spacing type" << as_numeric(space)<< "not implemented";
    return S57::PaintDataMap();
  }

  const TextData d = TextManager::instance()->textData(txt, weight/*, space*/);
  if (!d.isValid()) {
    return S57::PaintDataMap();
  }

  auto hjust = as_enum<TXT::HJust>(vals[1].toUInt(), TXT::AllHjusts);
  auto vjust = as_enum<TXT::VJust>(vals[2].toUInt(), TXT::AllVjusts);

  const QPointF dPivot(m_pivotHMap[hjust] * d.bbox().width(),
                       m_pivotVMap[vjust] * d.bbox().height());

  // ad-hoc factor: too large fonts
  const float bodySizeMM = chars.mid(3).toUInt() * .351 * .6;

  const QPointF dMM = QPointF(vals[5].toInt(), - vals[6].toInt()) * bodySizeMM;

  const QColor color = S52::GetColor(vals[7].toUInt());

  S57::PaintData* p = new S57::TextElemData(obj->geometry()->center(),
                                            d.bbox().bottomLeft(), // inverted y-axis
                                            dPivot,
                                            dMM,
                                            bodySizeMM / d.bbox().height(),
                                            d.offset(),
                                            d.elements(),
                                            color);

  return S57::PaintDataMap{{p->type(), p}};
}


S57::PaintDataMap S52::TextExtended::execute(const QVector<QVariant>& vals,
                                             const S57::Object* obj) {
  const QString txt = vals[0].toString();
  if (txt.isEmpty()) {
    return S57::PaintDataMap();
  }

  const int numAttrs = vals[1].toInt();

  char format[256];
  strcpy(format, txt.toUtf8().data());
  char buf[256];
  for (int i = 0; i < numAttrs; i++) {
    const QMetaType::Type t = static_cast<QMetaType::Type>(vals[2 + i].type());
    switch (t) {
    case QMetaType::QString:
      sprintf(buf, format, vals[2 + i].toString().toUtf8().constData());
      break;
    case QMetaType::Double:
      sprintf(buf, format, vals[2 + i].toDouble());
      break;
    case QMetaType::Int:
      sprintf(buf, format, vals[2 + i].toInt());
      break;
    default:
      qWarning() << "TE: Unhandled attribute value" << vals[2 + i];
      qWarning() << "[Class]" << S52::GetClassInfo(obj->classCode());
      qWarning() << "[Location]" << obj->geometry()->centerLL().print();
      for (auto k: obj->attributes().keys()) {
        qWarning() << GetAttributeInfo(k, obj);
      }
      return S57::PaintDataMap();
    }
    strcpy(format, buf);
  }
  // qDebug() << "TE:" << as_numeric(obj->geometry()->type()) << buf;

  QVector<QVariant> txVals;
  txVals.append(QVariant::fromValue(QString::fromUtf8(buf)));
  for (int i = 0; i < 8; i++) {
    txVals.append(vals[numAttrs + 2 + i]);
  }

  return S52::FindFunction("TX")->execute(vals, obj);
}


S52::CSDepthArea01::CSDepthArea01(quint32 index)
  : S52::Function("DEPARE01", index)
  , m_drval1(S52::FindIndex("DRVAL1"))
  , m_drval2(S52::FindIndex("DRVAL2"))
  , m_depdw(S52::FindIndex("DEPDW"))
  , m_depmd(S52::FindIndex("DEPMD"))
  , m_depms(S52::FindIndex("DEPMS"))
  , m_depvs(S52::FindIndex("DEPVS"))
  , m_depit(S52::FindIndex("DEPIT"))
  , m_drgare(S52::FindIndex("DRGARE"))
  , m_drgare01(S52::FindIndex("DRGARE01"))
  , m_chgrf(S52::FindIndex("CHGRF")) {}

S57::PaintDataMap S52::CSDepthArea01::execute(const QVector<QVariant>&,
                                              const S57::Object* obj) {

  auto geom = dynamic_cast<const S57::Geometry::Area*>(obj->geometry());
  Q_ASSERT(geom != nullptr);

  // Determine the color based on mariner selections

  bool ok;
  double depth = -1.;
  const double drval1 = obj->attributeValue(m_drval1).toDouble(&ok);
  if (ok) {
    depth = drval1;
  }
  const double drval2 = obj->attributeValue(m_drval2).toDouble(&ok);
  if (ok && drval2 < depth) {
    depth = drval2;
  }


  S57::PaintData* p;

  const Settings* cfg = Settings::instance();

  QColor color;
  if (cfg->twoShades() && depth >= cfg->safetyContour()) {
    color = S52::GetColor(m_depdw);
  } else if (depth >= cfg->deepContour()) {
    color = S52::GetColor(m_depdw);
  } else if (depth >= cfg->safetyContour()) {
    color = S52::GetColor(m_depmd);
  } else if (depth >= cfg->shallowContour()) {
    color = S52::GetColor(m_depms);
  } else if (depth >= 0) {
    color = S52::GetColor(m_depvs);
  } else {
    color = S52::GetColor(m_depit);
  }

  if (obj->classCode() != m_drgare) {
    if (geom->indexed()) {
      p = new S57::TriangleElemData(geom->triangleElements(), geom->vertexOffset(), color);
    } else {
      p = new S57::TriangleArrayData(geom->triangleElements(), geom->vertexOffset(), color);
    }
    return S57::PaintDataMap{{p->type(), p}};
  }

  if (!obj->attributeValue(m_drval1).isValid()) {
    color = S52::GetColor(m_depmd);
  }

  if (geom->indexed()) {
    p = new S57::TriangleElemData(geom->triangleElements(), geom->vertexOffset(), color);
  } else {
    p = new S57::TriangleArrayData(geom->triangleElements(), geom->vertexOffset(), color);
  }

  S57::PaintDataMap ps{{p->type(), p}};

  // run AP(DRGARE01);LS(DASH,1,CHGRF)

  QVector<QVariant> vals;
  vals.append(QVariant::fromValue(m_drgare01));
  vals.append(QVariant::fromValue(0.));
  ps += S52::FindFunction("AP")->execute(vals, obj);

  vals.clear();
  vals.append(QVariant::fromValue(int(S52::Lookup::Line::Dashed)));
  vals.append(QVariant::fromValue(1));
  vals.append(QVariant::fromValue(m_chgrf));
  ps += S52::FindFunction("LS")->execute(vals, obj);

  return ps;
}

S57::PaintDataMap S52::CSResArea02::execute(const QVector<QVariant>&,
                                            const S57::Object* /*obj*/) {
  qWarning() << "RESARE02: not implemented";
  return S57::PaintDataMap(); // invalid paint data
}

S52::CSDataCov01::CSDataCov01(quint32 index)
  : Function("DATCVR01", index)
  , m_hodata01(S52::FindIndex("HODATA01")) {}

S57::PaintDataMap S52::CSDataCov01::execute(const QVector<QVariant>&,
                                            const S57::Object* obj) {
  QVector<QVariant> vals;
  vals.append(QVariant::fromValue(m_hodata01));
  return S52::FindFunction("LC")->execute(vals, obj);
}

S57::PaintDataMap S52::CSDepthArea02::execute(const QVector<QVariant>&,
                                            const S57::Object* /*obj*/) {
  qWarning() << "DEPARE02: not implemented";
  return S57::PaintDataMap(); // invalid paint data
}

S57::PaintDataMap S52::CSDepthContours02::execute(const QVector<QVariant>&,
                                            const S57::Object* /*obj*/) {
  qWarning() << "DEPCNT02: not implemented";
  return S57::PaintDataMap(); // invalid paint data
}

S52::CSLights05::CSLights05(quint32 index)
  : Function("LIGHTS05", index)
  , m_catlit(S52::FindIndex("CATLIT"))
  , m_lights82(S52::FindIndex("LIGHTS82"))
  , m_lights81(S52::FindIndex("LIGHTS81"))
  , m_lights11(S52::FindIndex("LIGHTS11"))
  , m_lights12(S52::FindIndex("LIGHTS12"))
  , m_lights13(S52::FindIndex("LIGHTS13"))
  , m_litdef11(S52::FindIndex("LITDEF11"))
  , m_chblk(S52::FindIndex("CHBLK"))
  , m_colour(S52::FindIndex("COLOUR"))
  , m_sectr1(S52::FindIndex("SECTR1"))
  , m_sectr2(S52::FindIndex("SECTR2"))
  , m_orient(S52::FindIndex("ORIENT"))
  , m_quesmrk1(S52::FindIndex("QUESMRK1"))
  , m_set_wyo({1, 6, 11})
  , m_set_wr({1, 3})
  , m_set_r({3})
  , m_set_wg({1, 4})
  , m_set_g({4})
  , m_set_o({11})
  , m_set_y({6})
  , m_set_w({1})
{}

static QString litdsn01(const S57::Object* /*obj*/) {
  return "FIXME";
}

static bool overlaps_and_smaller(float s1, float s2, const QVariant& v1, const QVariant& v2) {
  const bool isSector = v1.isValid() && v2.isValid();
  if (!isSector) return false;
  float s3 = v1.toFloat();
  float s4 = v2.toFloat();
  while (s4 < s3) s4 += 360.;
  auto r1 = QRectF(QPointF(s1, 0.), QPointF(s2, 1.));
  auto r2 = QRectF(QPointF(s3, 0.), QPointF(s4, 1.));
  return r1.intersects(r2) && s4 - s3 > s2 - s1;
}

S57::PaintDataMap S52::CSLights05::execute(const QVector<QVariant>&,
                                            const S57::Object* obj) {
  QSet<int> catlit;
  auto items = obj->attributeValue(m_catlit).toList();
  for (auto i: items) catlit.insert(i.toUInt());

  if (!catlit.isEmpty()) {
    if (catlit.contains(floodlight) || catlit.contains(spotlight)) {
      QVector<QVariant> vals;
      vals.append(QVariant::fromValue(m_lights82));
      return S52::FindFunction("SY")->execute(vals, obj);
    }
    if (catlit.contains(striplight)) {
      QVector<QVariant> vals;
      vals.append(QVariant::fromValue(m_lights81));
      return S52::FindFunction("SY")->execute(vals, obj);
    }
  }

  QSet<int> cols;
  items = obj->attributeValue(m_colour).toList();
  if (items.isEmpty()) {
    cols.insert(magenta);
  } else {
    for (auto i: items) cols.insert(i.toInt());
  }

  quint32 lightSymbol;
  if (cols == m_set_wr || cols == m_set_w) {
    lightSymbol = m_lights11;
  } else if (cols == m_set_wg || cols == m_set_g) {
    lightSymbol = m_lights12;
  } else if (cols == m_set_o || cols == m_set_y || cols == m_set_w) {
    lightSymbol = m_lights13;
  } else {
    lightSymbol = m_litdef11;
  }


  // continuation A


  const bool isSector = obj->attributeValue(m_sectr1).isValid() && obj->attributeValue(m_sectr2).isValid();

  if (!isSector) {
    bool flare_at_45 = false;
    S57::Object::LocationIterator it = obj->others();
    for (; it != obj->othersEnd() && it.key() == obj->geometry()->centerLL(); ++it) {
      if (it.value() == obj) continue;
      if (cols.intersects(m_set_wyo)) flare_at_45 = true;
      break;
    }

    QVector<QVariant> vals;
    vals.append(QVariant::fromValue(lightSymbol));

    S57::PaintDataMap ps;
    if (catlit.contains(directional) || catlit.contains(moire_effect)) {
      auto orient = obj->attributeValue(m_orient);
      if (orient.isValid()) {
        vals.append(orient);
        ps = S52::FindFunction("SY")->execute(vals, obj);
        vals.clear();
        vals.append(QVariant::fromValue(int(S52::Lookup::Line::Dashed)));
        vals.append(QVariant::fromValue(1));
        vals.append(QVariant::fromValue(m_chblk));
        ps += S52::FindFunction("LS")->execute(vals, obj);
      } else {
        vals.clear();
        vals.append(QVariant::fromValue(m_quesmrk1));
        ps = S52::FindFunction("SY")->execute(vals, obj);
      }
    } else {
      vals.append(QVariant::fromValue(flare_at_45 ? 45. : 135.));
      ps = S52::FindFunction("SY")->execute(vals, obj);
    }
    if (Settings::instance()->textGrouping().contains(23)) {
      vals.clear();
      vals.append(QVariant::fromValue(litdsn01(obj)));
      vals.append(QVariant::fromValue(3));
      vals.append(QVariant::fromValue(flare_at_45 ? 1 : 2));
      vals.append(QVariant::fromValue(3));
      vals.append(QVariant::fromValue(QString("15110")));
      vals.append(QVariant::fromValue(2));
      vals.append(QVariant::fromValue(flare_at_45 ? -1 : 0));
      vals.append(QVariant::fromValue(m_chblk));
      vals.append(QVariant::fromValue(23));
      ps += S52::FindFunction("TX")->execute(vals, obj);
    }

    return ps;
  }

  // Continuation B
  auto s1 = obj->attributeValue(m_sectr1).toFloat();
  auto s2 = obj->attributeValue(m_sectr2).toFloat();
  while (s2 < s1) s2 += 360.;
  const bool allRound = s2 - s1 < 1. || std::abs(s2 - s1 - 360.) < 1.;

  if (allRound) {
    QVector<QVariant> vals;
    vals.append(QVariant::fromValue(lightSymbol));
    vals.append(QVariant::fromValue(135.));
    auto ps = S52::FindFunction("SY")->execute(vals, obj);

    if (Settings::instance()->textGrouping().contains(23)) {
      vals.clear();
      vals.append(QVariant::fromValue(litdsn01(obj)));
      vals.append(QVariant::fromValue(3));
      vals.append(QVariant::fromValue(2));
      vals.append(QVariant::fromValue(3));
      vals.append(QVariant::fromValue(QString("15110")));
      vals.append(QVariant::fromValue(2));
      vals.append(QVariant::fromValue(0));
      vals.append(QVariant::fromValue(m_chblk));
      vals.append(QVariant::fromValue(23));
      ps += S52::FindFunction("TX")->execute(vals, obj);
    }

    return ps;
  }
  // Sector light

  // draw sector lines
  QVector<QVariant> vals;
  vals.append(QVariant::fromValue(int(S52::Lookup::Line::Dashed)));
  vals.append(QVariant::fromValue(1));
  vals.append(QVariant::fromValue(m_chblk));
  auto ps = S52::FindFunction("LS")->execute(vals, obj);

  //bool extended_arc_radius = false;
  S57::Object::LocationIterator it = obj->others();
  for (; it != obj->othersEnd() && it.key() == obj->geometry()->centerLL(); ++it) {
    if (it.value() == obj) continue;
    if (overlaps_and_smaller(s1, s2, it.value()->attributeValue(m_sectr1), it.value()->attributeValue(m_sectr2))) {
      //extended_arc_radius = true;
      break;
    }
  }

  return ps;
}

S57::PaintDataMap S52::CSObstruction04::execute(const QVector<QVariant>&,
                                                const S57::Object* /*obj*/) {
  qWarning() << "OBSTRN04: not implemented";
  return S57::PaintDataMap(); // invalid paint data
}

S52::CSQualOfPos01::CSQualOfPos01(quint32 index)
  : S52::Function("QUAPOS01", index)
  , m_quapos(S52::FindIndex("QUAPOS"))
  , m_lowacc21(S52::FindIndex("LOWACC21"))
  , m_coalne(S52::FindIndex("COALNE"))
  , m_conrad(S52::FindIndex("CONRAD"))
  , m_cstln(S52::FindIndex("CSTLN"))
  , m_chmgf(S52::FindIndex("CHMGF"))
  , m_quapos01(S52::FindIndex("QUAPOS01"))
  , m_quapos02(S52::FindIndex("QUAPOS02"))
  , m_quapos03(S52::FindIndex("QUAPOS03"))
  , m_lowacc03(S52::FindIndex("LOWACC01")) // was LOWACC03, but missing in chart symbols
{}

S57::PaintDataMap S52::CSQualOfPos01::execute(const QVector<QVariant>&,
                                              const S57::Object* obj) {
  QVector<QVariant> vals;

  const QVariant quapos = obj->attributeValue(m_quapos);
  const int v = quapos.toInt();
  const bool inaccurate = quapos.isValid() && v >= 2 && v < 10;

  switch (obj->geometry()->type()) {

  case S57::Geometry::Type::Line: {
    if (inaccurate) {
      vals.append(QVariant::fromValue(m_lowacc21));
      return S52::FindFunction("LC")->execute(vals, obj);
    }

    if (!quapos.isValid()) {

      vals.append(QVariant::fromValue(int(S52::Lookup::Line::Solid)));

      if (obj->classCode() == m_coalne) {
        QVariant conrad = obj->attributeValue(m_conrad);
        if (conrad.isValid()) {
          int v = conrad.toInt();
          if (v == 1) {
            vals.append(QVariant::fromValue(3));
            vals.append(QVariant::fromValue(m_chmgf));
          }
        }
      }

      if (vals.size() == 1) {
        vals.append(QVariant::fromValue(1));
        vals.append(QVariant::fromValue(m_cstln));
      }

      return S52::FindFunction("LS")->execute(vals, obj);
    }
    return S57::PaintDataMap();
  }

  case S57::Geometry::Type::Point: {
    if (inaccurate) {
      switch (v) {
      case 4:
        vals.append(QVariant::fromValue(m_quapos01)); break; // "PA"
      case 5:
        vals.append(QVariant::fromValue(m_quapos02)); break; // "PD"
      case 7:
      case 8:
        vals.append(QVariant::fromValue(m_quapos03)); break; // "REP"
      default:
        vals.append(QVariant::fromValue(m_lowacc03)); // "?"
      }
    }
    return S52::FindFunction("SY")->execute(vals, obj);
  }
  default:
    return S57::PaintDataMap();
  }
}

S52::CSRestrEntry01::CSRestrEntry01(quint32 index)
  : Function("RESTRN01", index)
  , m_restrn(S52::FindIndex("RESTRN"))
  , m_entres51(S52::FindIndex("ENTRES51"))
  , m_entres61(S52::FindIndex("ENTRES61"))
  , m_entres71(S52::FindIndex("ENTRES71"))
  , m_achres51(S52::FindIndex("ACHRES51"))
  , m_achres61(S52::FindIndex("ACHRES61"))
  , m_achres71(S52::FindIndex("ACHRES71"))
  , m_fshres51(S52::FindIndex("FSHRES51"))
  , m_fshres71(S52::FindIndex("FSHRES71"))
  , m_infare51(S52::FindIndex("INFARE51"))
  , m_rsrdef51(S52::FindIndex("RSRDEF51"))
  , m_set1({1, 2})
  , m_set2({3, 4, 5, 6})
  , m_set3({7, 8, 14})
  , m_set4({9, 10, 11, 12, 13})
{}

S57::PaintDataMap S52::CSRestrEntry01::execute(const QVector<QVariant>&,
                                            const S57::Object* obj) {
  const QVariant restrn = obj->attributeValue(m_restrn);
  if (!restrn.isValid()) return S57::PaintDataMap();

  auto items = restrn.toList();
  if (items.isEmpty()) return S57::PaintDataMap();
  QSet<int> s;
  for (auto i: items) s.insert(i.toInt());

  QVector<QVariant> vals;
  if (s.intersects(m_set3)) {
    // continuation A
    if (s.intersects(m_set1) || s.intersects(m_set2)) {
      vals.append(QVariant::fromValue(m_entres61));
    } else if (s.intersects(m_set4)) {
      vals.append(QVariant::fromValue(m_entres71));
    } else {
      vals.append(QVariant::fromValue(m_entres51));
    }
  } else if (s.intersects(m_set1)) {
    // continuation B
    if (s.intersects(m_set2)) {
      vals.append(QVariant::fromValue(m_achres61));
    } else if (s.intersects(m_set4)) {
      vals.append(QVariant::fromValue(m_achres71));
    } else {
      vals.append(QVariant::fromValue(m_achres51));
    }
  } else if (s.intersects(m_set2)) {
    // continuation C
    if (s.intersects(m_set4)) {
      vals.append(QVariant::fromValue(m_fshres71));
    } else {
      vals.append(QVariant::fromValue(m_fshres51));
    }
  } else if (s.intersects(m_set4)) {
    vals.append(QVariant::fromValue(m_infare51));
  } else {
    vals.append(QVariant::fromValue(m_rsrdef51));
  }
  return S52::FindFunction("SY")->execute(vals, obj);
}


S52::CSShorelineQualOfPos03::CSShorelineQualOfPos03(quint32 index)
  : Function("SLCONS03", index)
  , m_quapos(S52::FindIndex("QUAPOS"))
  , m_lowacc01(S52::FindIndex("LOWACC01"))
  , m_crossx01(S52::FindIndex("CROSSX01"))
  , m_condtn(S52::FindIndex("CONDTN"))
  , m_cstln(S52::FindIndex("CSTLN"))
  , m_catslc(S52::FindIndex("CATSLC"))
  , m_watlev(S52::FindIndex("WATLEV")) {}

S57::PaintDataMap S52::CSShorelineQualOfPos03::execute(const QVector<QVariant>&,
                                                       const S57::Object* obj) {

  const QVariant quapos = obj->attributeValue(m_quapos);
  const int v = quapos.toInt();
  const bool inaccurate = quapos.isValid() && v >= 2 && v < 10;

  if (obj->geometry()->type() == S57::Geometry::Type::Meta) return S57::PaintDataMap();

  if (obj->geometry()->type() == S57::Geometry::Type::Point) {
    if (inaccurate) {
      QVector<QVariant> vals;
      vals.append(QVariant::fromValue(m_lowacc01));
      return S52::FindFunction("SY")->execute(vals, obj);
    }
    return S57::PaintDataMap();
  }

  S57::PaintDataMap ps;
  if (obj->geometry()->type() == S57::Geometry::Type::Area) {
    QVector<QVariant> vals;
    vals.append(QVariant::fromValue(m_crossx01));
    vals.append(QVariant::fromValue(0.));
    ps = S52::FindFunction("AP")->execute(vals, obj);
  }

  if (inaccurate) {
    QVector<QVariant> vals;
    vals.append(QVariant::fromValue(m_lowacc01));
    ps += S52::FindFunction("LC")->execute(vals, obj);
  } else {
    const QVariant condtn = obj->attributeValue(m_condtn);
    const int v1 = condtn.toInt();

    QVector<QVariant> vals;

    if (condtn.isValid() && (v1 == 1 || v1 == 2)) {
      vals.append(QVariant::fromValue(int(S52::Lookup::Line::Dashed)));
      vals.append(QVariant::fromValue(1));
    } else {
      const QVariant catslc = obj->attributeValue(m_catslc);
      const int v2 = catslc.toInt();

      if (catslc.isValid() && (v2 == 6 || v2 == 15 || v2 == 16)) { // Some sort of wharf
        vals.append(QVariant::fromValue(int(S52::Lookup::Line::Solid)));
        vals.append(QVariant::fromValue(4));
      } else {
        const QVariant watlev = obj->attributeValue(m_watlev);
        const int v3 = watlev.toInt();

        if (watlev.isValid() && v3 == 2) {
          vals.append(QVariant::fromValue(int(S52::Lookup::Line::Solid)));
          vals.append(QVariant::fromValue(2));
        } else if (watlev.isValid() && (v3 == 3 || v3 == 4)) {
          vals.append(QVariant::fromValue(int(S52::Lookup::Line::Dashed)));
          vals.append(QVariant::fromValue(2));
        } else {
          vals.append(QVariant::fromValue(int(S52::Lookup::Line::Solid)));
          vals.append(QVariant::fromValue(2));
        }
      }

      if (!vals.isEmpty()) {
        vals.append(QVariant::fromValue(m_cstln));
        ps += S52::FindFunction("LS")->execute(vals, obj);
      }
    }
  }

  return ps;
}

S57::PaintDataMap S52::CSEntrySoundings02::execute(const QVector<QVariant>&,
                                                   const S57::Object* /*obj*/) {
  qWarning() << "SOUNDG02: not implemented";
  return S57::PaintDataMap(); // invalid paint data
}

S52::CSTopmarks01::CSTopmarks01(quint32 index)
  : Function("TOPMAR01", index)
  , m_quesmrk1(S52::FindIndex("QUESMRK1"))
  , m_topshp(S52::FindIndex("TOPSHP"))
  , m_tmardef1(S52::FindIndex("TMARDEF1"))
  , m_tmardef2(S52::FindIndex("TMARDEF2"))
  , m_set_floats({S52::FindIndex("LITFLT"),
                 S52::FindIndex("LITVES"),
                 S52::FindIndex("BOYCAR"),
                 S52::FindIndex("BOYINB"),
                 S52::FindIndex("BOYISD"),
                 S52::FindIndex("BOYLAT"),
                 S52::FindIndex("BOYSAW"),
                 S52::FindIndex("BOYSPP"),
                 S52::FindIndex("boylat"),
                 S52::FindIndex("boywtw"),
                 })
  , m_set_rigids({S52::FindIndex("LITFLT"),
                 S52::FindIndex("BCNCAR"),
                 S52::FindIndex("BCNISD"),
                 S52::FindIndex("BCNLAT"),
                 S52::FindIndex("BCNSAW"),
                 S52::FindIndex("BCNSPP"),
                 S52::FindIndex("bcnlat"),
                 S52::FindIndex("bcnwtw"),
                 })
  , m_floats ({
{1, S52::FindIndex("TOPMAR02")}, {2, S52::FindIndex("TOPMAR04")},
{3, S52::FindIndex("TOPMAR10")}, {4, S52::FindIndex("TOPMAR12")},
{5, S52::FindIndex("TOPMAR13")}, {6, S52::FindIndex("TOPMAR14")},
{7, S52::FindIndex("TOPMAR65")}, {8, S52::FindIndex("TOPMAR17")},
{9, S52::FindIndex("TOPMAR16")}, {10, S52::FindIndex("TOPMAR08")},
{11, S52::FindIndex("TOPMAR07")}, {12, S52::FindIndex("TOPMAR14")},
{13, S52::FindIndex("TOPMAR05")}, {14, S52::FindIndex("TOPMAR06")},
{18, S52::FindIndex("TOPMAR10")}, {19, S52::FindIndex("TOPMAR13")},
{20, S52::FindIndex("TOPMAR14")}, {21, S52::FindIndex("TOPMAR13")},
{22, S52::FindIndex("TOPMAR14")}, {23, S52::FindIndex("TOPMAR14")},
{24, S52::FindIndex("TOPMAR02")}, {25, S52::FindIndex("TOPMAR04")},
{26, S52::FindIndex("TOPMAR10")}, {27, S52::FindIndex("TOPMAR17")},
{28, S52::FindIndex("TOPMAR18")}, {29, S52::FindIndex("TOPMAR02")},
{30, S52::FindIndex("TOPMAR17")}, {31, S52::FindIndex("TOPMAR14")},
{32, S52::FindIndex("TOPMAR10")},
              })
  , m_rigids({
{1, S52::FindIndex("TOPMAR22")}, {2, S52::FindIndex("TOPMAR24")},
{3, S52::FindIndex("TOPMAR30")}, {4, S52::FindIndex("TOPMAR32")},
{5, S52::FindIndex("TOPMAR33")}, {6, S52::FindIndex("TOPMAR34")},
{7, S52::FindIndex("TOPMAR85")}, {8, S52::FindIndex("TOPMAR86")},
{9, S52::FindIndex("TOPMAR36")}, {10, S52::FindIndex("TOPMAR28")},
{11, S52::FindIndex("TOPMAR27")}, {12, S52::FindIndex("TOPMAR14")},
{13, S52::FindIndex("TOPMAR25")}, {14, S52::FindIndex("TOPMAR26")},
{15, S52::FindIndex("TOPMAR88")}, {16, S52::FindIndex("TOPMAR87")},
{18, S52::FindIndex("TOPMAR30")}, {19, S52::FindIndex("TOPMAR33")},
{20, S52::FindIndex("TOPMAR34")}, {21, S52::FindIndex("TOPMAR33")},
{22, S52::FindIndex("TOPMAR34")}, {23, S52::FindIndex("TOPMAR34")},
{24, S52::FindIndex("TOPMAR22")}, {25, S52::FindIndex("TOPMAR24")},
{26, S52::FindIndex("TOPMAR30")}, {27, S52::FindIndex("TOPMAR86")},
{28, S52::FindIndex("TOPMAR89")}, {29, S52::FindIndex("TOPMAR22")},
{30, S52::FindIndex("TOPMAR86")}, {31, S52::FindIndex("TOPMAR14")},
{32, S52::FindIndex("TOPMAR30")},
             })

{}

S57::PaintDataMap S52::CSTopmarks01::execute(const QVector<QVariant>&,
                                             const S57::Object* obj) {
  quint32 topmrk = m_quesmrk1;

  if (obj->attributeValue(m_topshp).isValid()) {
    quint32 topshp = obj->attributeValue(m_topshp).toUInt();

    S57::Object::LocationIterator it = obj->others();
    for (; it != obj->othersEnd() && it.key() == obj->geometry()->centerLL(); ++it) {
      if (it.value() == obj) continue;
      if (isFloating(it.value())) {
        if (m_floats.contains(topshp)) {
          topmrk = m_floats[topshp];
        } else {
          topmrk = m_tmardef2;
        }
        qDebug() << "TOPMRK01: floating";
        break;
      }
      if (isRigid(it.value())) {
        if (m_rigids.contains(topshp)) {
          topmrk = m_rigids[topshp];
        } else {
          topmrk = m_tmardef1;
        }
        qDebug() << "TOPMRK01: rigid";
        break;
      }
    }
  }

  QVector<QVariant> vals;
  vals.append(QVariant::fromValue(topmrk));
  vals.append(QVariant::fromValue(0.));
  return S52::FindFunction("SY")->execute(vals, obj);
}

bool S52::CSTopmarks01::isFloating(const S57::Object *obj) const {
  const quint32 cl = obj->classCode();
  return m_set_floats.contains(cl);
}

bool S52::CSTopmarks01::isRigid(const S57::Object *obj) const {
  const quint32 cl = obj->classCode();
  return m_set_rigids.contains(cl);
}

S57::PaintDataMap S52::CSWrecks02::execute(const QVector<QVariant>&,
                                            const S57::Object* /*obj*/) {
  qWarning() << "WRECKS02: not implemented";
  return S57::PaintDataMap(); // invalid paint data
}

