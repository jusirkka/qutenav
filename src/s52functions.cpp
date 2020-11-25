#include "s52functions.h"
#include "s52presentation.h"
#include "conf_marinerparams.h"
#include "settings.h"
#include <cmath>

static void mergePaintData(S57::PaintDataMap& ps, const S57::PaintDataMap& ps1) {
  for (auto it = ps1.constBegin(); it != ps1.constEnd(); ++it) {
    if (ps.contains(it.key())) {
      qWarning() << "Overwriting paint data" << quint8(it.key());
    }
    ps[it.key()] = it.value();
  }
}

S57::PaintDataMap S52::AreaColor::execute(const QVector<QVariant>& vals,
                                       const S57::Object* obj) {
  S57::PaintData p;

  auto geom = dynamic_cast<const S57::Geometry::Area*>(obj->geometry());
  Q_ASSERT(geom != nullptr);

  p.type = S57::PaintData::Type::TriangleArrays;
  p.elements = geom->triangleElements();
  p.vertexOffset = geom->vertexOffset();

  p.color = S52::GetColor(vals[0].toUInt());
  p.color.setAlpha(vals[1].toUInt());

  return S57::PaintDataMap{{p.type, p}};
}

S57::PaintDataMap S52::AreaPattern::execute(const QVector<QVariant>& /*vals*/,
                                            const S57::Object* /*obj*/) {
  // qWarning() << "AP: not implemented";
  return S57::PaintDataMap(); // invalid paint data
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

  S57::PaintData p;
  p.type = S57::PaintData::Type::LineElements;
  p.vertexOffset = 0;
  p.elements = line->lineElements();
  p.params.line.pattern = vals[0].toInt();
  p.params.line.lineWidth = vals[1].toInt();
  p.color = S52::GetColor(vals[2].toUInt());
  return S57::PaintDataMap{{p.type, p}};
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

  S57::PaintData p;
  p.type = S57::PaintData::Type::LineArrays;

  const GLfloat x1 = point->points()[0];
  const GLfloat y1 = point->points()[1];
  const GLfloat x2 = x1 - valnmr * sin(orient);
  const GLfloat y2 = y1 - valnmr * cos(orient);

  p.vertices.append(2 * x1 - x2);
  p.vertices.append(2 * y1 - y2);
  p.vertices.append(x1);
  p.vertices.append(y1);
  p.vertices.append(x2);
  p.vertices.append(y2);
  p.vertices.append(2 * x2 - x1);
  p.vertices.append(2 * y2 - y1);

  S57::ElementData e;
  e.elementCount = 4;
  e.elementOffset = 0;
  e.mode = GL_LINE_STRIP_ADJACENCY;
  p.elements.append(e);

  p.params.line.pattern = vals[0].toInt();
  p.params.line.lineWidth = vals[1].toInt();
  p.color = S52::GetColor(vals[2].toUInt());
  return p;
}


S57::PaintDataMap S52::LineComplex::execute(const QVector<QVariant>& /*vals*/,
                                            const S57::Object* /*obj*/) {
  // qWarning() << "LC: not implemented";
  return S57::PaintDataMap(); // invalid paint data
}


S57::PaintDataMap S52::PointSymbol::execute(const QVector<QVariant>& /*vals*/,
                                            const S57::Object* /*obj*/) {
  // qWarning() << "SY: not implemented";
  return S57::PaintDataMap(); // invalid paint data
}

S57::PaintDataMap S52::Text::execute(const QVector<QVariant>& /*vals*/,
                                            const S57::Object* /*obj*/) {
  // qWarning() << "TX: not implemented";
  return S57::PaintDataMap(); // invalid paint data
}

S57::PaintDataMap S52::TextExtended::execute(const QVector<QVariant>& /*vals*/,
                                            const S57::Object* /*obj*/) {
  // qWarning() << "TE: not implemented";
  return S57::PaintDataMap(); // invalid paint data
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


  S57::PaintData p;

  p.type = S57::PaintData::Type::Triangles;
  p.elements = geom->triangleElements();
  p.vertexOffset = geom->vertexOffset();

  const Settings* cfg = Settings::instance();

  if (cfg->twoShades() && depth >= cfg->safetyContour()) {
    p.color = S52::GetColor(m_depdw);
  } else if (depth >= cfg->deepContour()) {
    p.color = S52::GetColor(m_depdw);
  } else if (depth >= cfg->safetyContour()) {
    p.color = S52::GetColor(m_depmd);
  } else if (depth >= cfg->shallowContour()) {
    p.color = S52::GetColor(m_depms);
  } else if (depth >= 0) {
    p.color = S52::GetColor(m_depvs);
  } else {
    p.color = S52::GetColor(m_depit);
  }

  if (obj->classCode() != m_drgare) {
    return S57::PaintDataMap{{p.type, p}};
  }

  if (!obj->attributeValue(m_drval1).isValid()) {
    p.color = S52::GetColor(m_depmd);
  }

  S57::PaintDataMap ps{{p.type, p}};

  // run AP(DRGARE01);LS(DASH,1,CHGRF)

  QVector<QVariant> vals;
  vals.append(QVariant::fromValue(m_drgare01));
  auto ps1 = S52::FindFunction("AP")->execute(vals, obj);

  mergePaintData(ps, ps1);

  vals.clear();
  vals.append(QVariant::fromValue(int(S52::Lookup::Line::Dashed)));
  vals.append(QVariant::fromValue(1));
  vals.append(QVariant::fromValue(m_chgrf));
  ps1 = S52::FindFunction("LS")->execute(vals, obj);

  mergePaintData(ps, ps1);

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

static QString litdsn01(const S57::Object* obj) {
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
  QSet<int> cols;
  items = obj->attributeValue(m_colour).toList();
  if (items.isEmpty()) {
    cols.insert(magenta);
  } else {
    for (auto i: items) cols.insert(i.toInt());
  }
  const bool isSector = obj->attributeValue(m_sectr1).isValid() && obj->attributeValue(m_sectr2).isValid();

  if (!isSector) {
    bool flare_at_45 = false;
    if (!obj->others().isEmpty() && cols.intersects(m_set_wyo)) {
      flare_at_45 = true;
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
        auto ps1 = S52::FindFunction("LS")->execute(vals, obj);
        mergePaintData(ps, ps1);
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
      vals.append(QVariant::fromValue("15110"));
      vals.append(QVariant::fromValue(2));
      vals.append(QVariant::fromValue(flare_at_45 ? -1 : 0));
      vals.append(QVariant::fromValue(m_chblk));
      vals.append(QVariant::fromValue(23));
      auto ps1 = S52::FindFunction("TX")->execute(vals, obj);
      mergePaintData(ps, ps1);
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
      vals.append(QVariant::fromValue("15110"));
      vals.append(QVariant::fromValue(2));
      vals.append(QVariant::fromValue(0));
      vals.append(QVariant::fromValue(m_chblk));
      vals.append(QVariant::fromValue(23));
      auto ps1 = S52::FindFunction("TX")->execute(vals, obj);
      mergePaintData(ps, ps1);
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

  bool extended_arc_radius = false;
  for (const S57::Object* other: obj->others()) {
    if (overlaps_and_smaller(s1, s2, other->attributeValue(m_sectr1), other->attributeValue(m_sectr2))) {
      extended_arc_radius = true;
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
    return S57::PaintDataMap();
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
    ps = S52::FindFunction("AP")->execute(vals, obj);
  }

  S57::PaintDataMap ps1;

  if (inaccurate) {
    QVector<QVariant> vals;
    vals.append(QVariant::fromValue(m_lowacc01));
    ps1 = S52::FindFunction("LC")->execute(vals, obj);
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
        ps1 = S52::FindFunction("LS")->execute(vals, obj);
      }
    }
  }

  mergePaintData(ps, ps1);

  return ps;
}

S57::PaintDataMap S52::CSEntrySoundings02::execute(const QVector<QVariant>&,
                                            const S57::Object* /*obj*/) {
  qWarning() << "SOUNDG02: not implemented";
  return S57::PaintDataMap(); // invalid paint data
}

S57::PaintDataMap S52::CSTopmarks01::execute(const QVector<QVariant>&,
                                            const S57::Object* /*obj*/) {
  qWarning() << "TOPMAR01: not implemented";
  return S57::PaintDataMap(); // invalid paint data
}

S57::PaintDataMap S52::CSWrecks02::execute(const QVector<QVariant>&,
                                            const S57::Object* /*obj*/) {
  qWarning() << "WRECKS02: not implemented";
  return S57::PaintDataMap(); // invalid paint data
}

