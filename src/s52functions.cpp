#include "s52functions.h"
#include "s52presentation.h"
#include "conf_marinerparams.h"
#include "settings.h"


S57::PaintDataMap S52::AreaColor::execute(const QVector<QVariant>& vals,
                                       const S57::Object* obj) {
  S57::PaintData p;

  auto geom = dynamic_cast<const S57::Geometry::Area*>(obj->geometry());
  Q_ASSERT(geom != nullptr);

  p.type = S57::PaintData::Type::Triangles;
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

S57::PaintDataMap S52::LineSimple::execute(const QVector<QVariant>& vals,
                                           const S57::Object* obj) {
  S57::PaintData p;
  p.type = S57::PaintData::Type::Lines;

  auto line = dynamic_cast<const S57::Geometry::Line*>(obj->geometry());
  Q_ASSERT(line != nullptr);

  p.elements = line->lineElements();
  // p.vertexOffset = line->vertexOffset();
  p.vertexOffset = 0;

  p.params.line.pattern = vals[0].toInt();
  p.params.line.lineWidth = vals[1].toInt();
  p.color = S52::GetColor(vals[2].toUInt());

  return S57::PaintDataMap{{p.type, p}};
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

  for (auto it = ps1.constBegin(); it != ps1.constEnd(); ++it) {
    if (ps.contains(it.key())) {
      qWarning() << "Overwriting paint data" << quint8(it.key());
    }
    ps[it.key()] = it.value();
  }

  vals.clear();
  vals.append(QVariant::fromValue(int(S52::Lookup::Line::Dashed)));
  vals.append(QVariant::fromValue(1));
  vals.append(QVariant::fromValue(m_chgrf));
  ps1 = S52::FindFunction("LS")->execute(vals, obj);

  for (auto it = ps1.constBegin(); it != ps1.constEnd(); ++it) {
    if (ps.contains(it.key())) {
      qWarning() << "Overwriting paint data" << quint8(it.key());
    }
    ps[it.key()] = it.value();
  }

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

S57::PaintDataMap S52::CSLights05::execute(const QVector<QVariant>&,
                                            const S57::Object* /*obj*/) {
  qWarning() << "LIGHTS05: not implemented";
  return S57::PaintDataMap(); // invalid paint data
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

S57::PaintDataMap S52::CSRestrEntry01::execute(const QVector<QVariant>&,
                                            const S57::Object* /*obj*/) {
  qWarning() << "RESTRN01: not implemented";
  return S57::PaintDataMap(); // invalid paint data
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

  for (auto it = ps1.constBegin(); it != ps1.constEnd(); ++it) {
    if (ps.contains(it.key())) {
      qWarning() << "Overwriting paint data" << quint8(it.key());
    }
    ps[it.key()] = it.value();
  }

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

