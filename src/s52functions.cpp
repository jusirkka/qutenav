#include "s52functions.h"
#include "s52presentation.h"
#include "conf_marinerparams.h"
#include "settings.h"
#include <cmath>
#include "types.h"
#include "textmanager.h"
#include "rastersymbolmanager.h"
#include "vectorsymbolmanager.h"

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

  SymbolData s;
  quint32 index = vals[0].toUInt();
  // rotations are specified clockwise
  const Angle rot = Angle::fromDegrees(- vals[1].toDouble());
  bool raster = false;
  if (!rot.isZero()) {
    s = VectorSymbolManager::instance()->symbolData(index, S52::SymbolType::Pattern);
  } else {
    s = RasterSymbolManager::instance()->symbolData(index, S52::SymbolType::Pattern);
    if (!s.isValid()) {
      s = VectorSymbolManager::instance()->symbolData(index, S52::SymbolType::Pattern);
    } else {
      raster = true;
    }
  }

  if (!s.isValid()) {
    qWarning() << "Missing pattern" << S52::GetSymbolInfo(index, S52::SymbolType::Pattern);
    return S57::PaintDataMap();
  }

  auto geom = dynamic_cast<const S57::Geometry::Area*>(obj->geometry());
  Q_ASSERT(geom != nullptr);

  S57::PaintData* p;
  if (raster) {
    p = new S57::RasterPatternPaintData(index,
                                        s.offset(),
                                        geom->triangleElements(),
                                        geom->vertexOffset(),
                                        geom->indexed(),
                                        obj->boundingBox(),
                                        s.advance(),
                                        s.element());
  } else {
    //    qDebug() << "[VectorPattern:Class]" << S52::GetClassInfo(obj->classCode());
    //    qDebug() << "[VectorPattern:Symbol]" << S52::GetSymbolInfo(index, S52::SymbolType::Pattern);
    //    qDebug() << "[VectorPattern:Location]" << obj->geometry()->centerLL().print();
    //    for (auto k: obj->attributes().keys()) {
    //      qDebug() << GetAttributeInfo(k, obj);
    //    }

    QT::ColorVector colors;
    for (const S52::Color& c: s.colors()) {
      auto color = S52::GetColor(c.index);
      color.setAlpha(255 - as_numeric(c.alpha) * 255 / 4);
      colors.append(color);
    }
    p = new S57::VectorPatternPaintData(index,
                                        geom->triangleElements(),
                                        geom->vertexOffset(),
                                        geom->indexed(),
                                        obj->boundingBox(),
                                        s.advance(),
                                        rot,
                                        colors,
                                        s.elements());
  }

  return S57::PaintDataMap{{p->type(), p}};

}

S57::PaintDataMap S52::LineSimple::execute(const QVector<QVariant>& vals,
                                           const S57::Object* obj) {
  auto line = dynamic_cast<const S57::Geometry::Line*>(obj->geometry());

  S57::PaintData* p;

  auto pattern = as_enum<S52::LineType>(vals[0].toUInt(), S52::AllLineTypes);
  auto width = vals[1].toUInt();
  auto color = S52::GetColor(vals[2].toUInt());

  if (pattern == S52::LineType::Solid) {
    p = new S57::SolidLineElemData(line->lineElements(), 0, color, width);
  } else {
    p = new S57::DashedLineElemData(line->lineElements(), 0, color, width,
                                    as_numeric(pattern));
  }

  return S57::PaintDataMap{{p->type(), p}};
}

S57::PaintDataMap S52::LineComplex::execute(const QVector<QVariant>& vals,
                                            const S57::Object* obj) {

  quint32 index = vals[0].toUInt();
  SymbolData s = VectorSymbolManager::instance()->symbolData(index, S52::SymbolType::LineStyle);

  if (!s.isValid()) {
    qWarning() << "Missing linestyle" << S52::GetSymbolInfo(index, S52::SymbolType::LineStyle);
    return S57::PaintDataMap();
  }

  auto geom = dynamic_cast<const S57::Geometry::Line*>(obj->geometry());
  Q_ASSERT(geom != nullptr);

  // qDebug() << "[LineStyle:Class]" << S52::GetClassInfo(obj->classCode()) << obj->classCode();
  // qDebug() << "[LineStyle:Symbol]" << S52::GetSymbolInfo(index, S52::SymbolType::LineStyle) << index;
  // for (auto k: obj->attributes().keys()) {
  //   qDebug() << GetAttributeInfo(k, obj);
  // }

  QT::ColorVector colors;
  for (const S52::Color& c: s.colors()) {
    auto color = S52::GetColor(c.index);
    color.setAlpha(255 - as_numeric(c.alpha) * 255 / 4);
    colors.append(color);
  }
  auto p = new S57::LineStylePaintData(index,
                                       geom->lineElements(),
                                       0,
                                       obj->boundingBox(),
                                       s.advance(),
                                       colors,
                                       s.elements());

  return S57::PaintDataMap{{p->type(), p}};
}


S57::PaintDataMap S52::PointSymbol::execute(const QVector<QVariant>& vals,
                                            const S57::Object* obj) {

  SymbolData s;
  quint32 index = vals[0].toUInt();
  // rotations are specified clockwise
  const Angle rot = Angle::fromDegrees(- vals[1].toDouble());
  bool raster = false;
  if (!rot.isZero()) {
    s = VectorSymbolManager::instance()->symbolData(index, S52::SymbolType::Single);
  } else {
    s = RasterSymbolManager::instance()->symbolData(index, S52::SymbolType::Single);
    if (!s.isValid()) {
      s = VectorSymbolManager::instance()->symbolData(index, S52::SymbolType::Single);
    } else {
      raster = true;
    }
  }

  if (!s.isValid()) {
    qWarning() << "Missing symbol" << S52::GetSymbolInfo(index, S52::SymbolType::Single);
    return S57::PaintDataMap();
  }

  S57::PaintData* p;
  if (raster) {
    p = new S57::RasterSymbolPaintData(index,
                                       s.offset(),
                                       obj->geometry()->center(),
                                       s.element());
  } else {

    QT::ColorVector colors;
    for (const S52::Color& c: s.colors()) {
      auto color = S52::GetColor(c.index);
      color.setAlpha(255 - as_numeric(c.alpha) * 255 / 4);
      colors.append(color);
    }

    p = new S57::VectorSymbolPaintData(index,
                                       obj->geometry()->center(),
                                       rot,
                                       colors,
                                       s.elements());
  }

  return S57::PaintDataMap{{p->type(), p}};
}

S57::PaintDataMap S52::Text::execute(const QVector<QVariant>& vals,
                                     const S57::Object* obj) {

  QString txt = vals[0].toString();
  if (txt.isEmpty()) {
    return S57::PaintDataMap();
  }
  // qDebug() << "TX:" << as_numeric(obj->geometry()->type()) << txt;

  const quint8 group = vals[8].toUInt();
  if (!Settings::instance()->textGrouping().contains(group)) {
    // qDebug() << "skipping TX in group" << group;
    return S57::PaintDataMap();
  }

  const QString chars = vals[4].toString();

  Q_ASSERT(chars.left(1).toUInt() == 1); // style: always system's default
  const quint8 width = chars.mid(2, 1).toUInt();
  Q_ASSERT(width == 1 || width == 2); // always roman slant

  auto weight = as_enum<TXT::Weight>(chars.mid(1, 1).toUInt(), TXT::AllWeights);

  auto space = as_enum<TXT::Space>(vals[3].toUInt(), TXT::AllSpaces);
  if (space != TXT::Space::Standard && space != TXT::Space::Wrapped) {
    qWarning() << "TX: text spacing type" << as_numeric(space)<< "not implemented";
    return S57::PaintDataMap();
  }
  // TODO: we do not actually wrap although TXT::Space::Wrapped is accepted
  if (txt.length() > 80) {
    qWarning() << "Cutting long text" << txt;
    txt = txt.mid(80);
  }
  const TextData d = TextManager::instance()->textData(txt, weight);
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
    case QMetaType::UnknownType: // Empty QVariant
      return S57::PaintDataMap();
    default:
      qWarning() << "TE: Unhandled attribute value" << vals[2 + i];
      //      qWarning() << "[Class]" << S52::GetClassInfo(obj->classCode());
      //      qWarning() << "[Location]" << obj->geometry()->centerLL().print();
      //      for (auto k: obj->attributes().keys()) {
      //        qWarning() << GetAttributeInfo(k, obj);
      //      }
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

  return S52::FindFunction("TX")->execute(txVals, obj);
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
  vals.append(QVariant::fromValue(int(S52::LineType::Dashed)));
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
  , m_litvis(S52::FindIndex("LITVIS"))
  , m_outlw(S52::FindIndex("OUTLW"))
  , m_litrd(S52::FindIndex("LITRD"))
  , m_litgn(S52::FindIndex("LITGN"))
  , m_lityw(S52::FindIndex("LITYW"))
  , m_chmgd(S52::FindIndex("CHMGD"))
  , m_valnmr(S52::FindIndex("VALNMR"))
  , m_litchr(S52::FindIndex("LITCHR"))
  , m_siggrp(S52::FindIndex("SIGGRP"))
  , m_sigper(S52::FindIndex("SIGPER"))
  , m_height(S52::FindIndex("HEIGHT"))
  , m_set_wyo({1, 6, 11})
  , m_set_wr({1, 3})
  , m_set_r({3})
  , m_set_wg({1, 4})
  , m_set_g({4})
  , m_set_o({11})
  , m_set_y({6})
  , m_set_w({1})
  , m_set_faint({3, 7, 8})
  , m_abbrev{{1, "F"}, {2, "Fl"}, {3, "LFl"}, {4, "Q"}, {5, "VQ"}, {6, "UQ"},
             {7, "Iso"}, {8, "Oc"}, {9, "IQ"}, {10, "IVQ"}, {11, "IUQ"},
             {12, "Mo"}, {13, "FFl"}, {14, "Fl+LFl"}, {15, "OcFl"}, {16, "FLFl"},
             {17, "AlOc"}, {18, "AlLFl"}, {19, "AlFl"}, {20, "Al"}, {25, "Q+LFl"},
             {26, "VQ+LFl"}, {27, "UQ+LFl"}, {28, "Al"}, {29, "AlF Fl"}}
{}

QString S52::CSLights05::litdsn01(const S57::Object* obj) const {

  QString ret;
  // Light Characteristic
  if (obj->attributeValue(m_litchr).isValid()) {
    uint litchr = obj->attributeValue(m_litchr).toUInt();
    if (m_abbrev.contains(litchr)) {
      ret += m_abbrev[litchr];
    }
  }
  // Signal Group
  bool sig = false;
  if (obj->attributeValue(m_siggrp).isValid()) {
    QString siggrp = obj->attributeValue(m_siggrp).toString();
    if (siggrp.length() > 2) {
      QStringList groups;
      for (int i = 1; i < siggrp.length(); i += 3) {
        // assuming less than 10 groups
        groups << siggrp.mid(i, 1);
      }
      if (groups.length() > 1 || groups.first() != "1") {
        ret += "(" + groups.join("+") + ")";
        sig = true;
      }
    }
  }
  if (!sig) {
    ret += " ";
  }

  // Colour
  if (!obj->attributeValue(m_sectr1).isValid() && obj->attributeValue(m_colour).isValid()) {
    auto items = obj->attributeValue(m_colour).toList();
    for (auto i: items) {
      switch (i.toInt()) {
      case 1: ret += "W"; break;
      case 3: ret += "R"; break;
      case 4: ret += "G"; break;
      case 6: ret += "Y"; break;
      default:  /* noop */ ;
      }
    }
    ret += " ";
  }

  auto addValue = [&ret, obj] (const QString& fmt, quint32 v) {
    if (obj->attributeValue(v).isValid()) {
      const double x = obj->attributeValue(v).toDouble();
      const int prec = x - floor(x) > 0. ? 1 : 0;
      ret += fmt.arg(x, 0, 'f', prec);
    }
  };

  // Signal Period
  addValue("%1s", m_sigper);
  // Height
  addValue("%1m", m_height);
  // Range
  addValue("%1M", m_valnmr);

  return ret;
}

static bool overlaps_and_smaller(float s1, float s2,
                                 const QVariant& v1, const QVariant& v2,
                                 float& smin) {
  const bool isSector = v1.isValid() && v2.isValid();
  if (!isSector) return false;
  float s3 = v1.toFloat();
  float s4 = v2.toFloat();
  while (s4 < s3) s4 += 360.;

  if (s3 < smin) smin = s3;

  auto r1 = QRectF(QPointF(s1, 0.), QPointF(s2, 1.));
  auto r2 = QRectF(QPointF(s3, 0.), QPointF(s4, 1.));
  return r1.intersects(r2) && s4 - s3 > s2 - s1;
}

S57::PaintDataMap S52::CSLights05::execute(const QVector<QVariant>&,
                                           const S57::Object* obj) {
  // qDebug() << "[CSLights05:Class]" << S52::GetClassInfo(obj->classCode()) << obj->classCode();
  // for (auto k: obj->attributes().keys()) {
  //   qDebug() << GetAttributeInfo(k, obj);
  //}

  QSet<int> catlit;
  auto items = obj->attributeValue(m_catlit).toList();
  for (auto i: items) catlit.insert(i.toUInt());

  if (!catlit.isEmpty()) {
    if (catlit.contains(floodlight) || catlit.contains(spotlight)) {
      QVector<QVariant> vals;
      vals.append(QVariant::fromValue(m_lights82));
      vals.append(QVariant::fromValue(0.));
      return S52::FindFunction("SY")->execute(vals, obj);
    }
    if (catlit.contains(striplight)) {
      QVector<QVariant> vals;
      vals.append(QVariant::fromValue(m_lights81));
      vals.append(QVariant::fromValue(0.));
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
  quint32 sectorColor;
  if (cols == m_set_wr || cols == m_set_r) {
    lightSymbol = m_lights11;
    sectorColor = m_litrd;
  } else if (cols == m_set_wg || cols == m_set_g) {
    lightSymbol = m_lights12;
    sectorColor = m_litgn;
  } else if (cols == m_set_o || cols == m_set_y || cols == m_set_w) {
    lightSymbol = m_lights13;
    sectorColor = m_lityw;
  } else {
    lightSymbol = m_litdef11;
    sectorColor = m_chmgd;
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
        ps += drawDirection(obj);
        vals.clear();
        vals.append(QVariant::fromValue(QString("%03.0lf deg")));
        vals.append(QVariant::fromValue(1)); // number of attributes
        vals.append(orient);
        vals.append(QVariant::fromValue(3));
        vals.append(QVariant::fromValue(3));
        vals.append(QVariant::fromValue(3));
        vals.append(QVariant::fromValue(QString("15110")));
        vals.append(QVariant::fromValue(3));
        vals.append(QVariant::fromValue(1));
        vals.append(QVariant::fromValue(m_chblk));
        vals.append(QVariant::fromValue(23));
        ps += S52::FindFunction("TE")->execute(vals, obj);
      } else {
        vals.clear();
        vals.append(QVariant::fromValue(m_quesmrk1));
        ps = S52::FindFunction("SY")->execute(vals, obj);
      }
    } else {
      vals.append(QVariant::fromValue(flare_at_45 ? 45. : 135.));
      ps = S52::FindFunction("SY")->execute(vals, obj);
    }
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

    return ps;
  }
  // Sector light

  // draw sector lines
  auto ps = drawSectors(obj);

  float arc_radius = 20;

  float smin = 360;
  S57::Object::LocationIterator it = obj->others();
  for (; it != obj->othersEnd() && it.key() == obj->geometry()->centerLL(); ++it) {
    if (it.value() == obj) {
      smin = s1;
      continue;
    }
    auto v1 = it.value()->attributeValue(m_sectr1);
    auto v2 = it.value()->attributeValue(m_sectr2);
    if (overlaps_and_smaller(s1, s2, v1, v2, smin)) {
      qDebug() << "Extended radius";
      arc_radius = 25;
      break;
    }
  }

  // show text for only one sector
  if (s1 == smin) {
    QVector<QVariant> vals;
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

  // draw sector arc
  QSet<int> litvis;
  items = obj->attributeValue(m_litvis).toList();
  for (auto i: items) litvis.insert(i.toInt());
  if (litvis.intersects(m_set_faint)) {
    ps += drawArc(obj, arc_radius, 1, S52::LineType::Dashed, m_chblk);
  } else {
    ps += drawArc(obj, arc_radius - S52::LineWidthMM(1.5), 1, S52::LineType::Solid, m_outlw);
    ps += drawArc(obj, arc_radius, 2, S52::LineType::Solid, sectorColor);
    ps += drawArc(obj, arc_radius + S52::LineWidthMM(1.5), 1, S52::LineType::Solid, m_outlw);
  }

  return ps;
}

S57::PaintDataMap S52::CSLights05::drawDirection(const S57::Object *obj) const {
  auto point = dynamic_cast<const S57::Geometry::Point*>(obj->geometry());
  Q_ASSERT(point != nullptr);

  bool ok;
  const double orient = obj->attributeValue(m_orient).toDouble(&ok) * M_PI / 180.;
  if (!ok) {
    qDebug() << "CSLights05::drawDirection: Orient not present";
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

  GL::VertexVector vertices;
  const GLfloat x0 = point->points()[0];
  const GLfloat y0 = point->points()[1];
  const GLfloat x1 = x0 - valnmr * sin(orient);
  const GLfloat y1 = y0 - valnmr * cos(orient);

  vertices << 2 * x0 - x1 << 2 * y0 - y1;
  vertices << x0 << y0;
  vertices << x1 << y1;
  vertices << 2 * x1 - x0 << 2 * y1 - y0;


  auto color = S52::GetColor(m_chblk);
  auto p = new S57::DashedLineLocalData(vertices, elements, color, 1,
                                        as_numeric(S52::LineType::Dashed),
                                        false, QPointF(x0, y0));

  return S57::PaintDataMap{{p->type(), p}};
}

S57::PaintDataMap S52::CSLights05::drawSectors(const S57::Object *obj) const {
  auto point = dynamic_cast<const S57::Geometry::Point*>(obj->geometry());
  Q_ASSERT(point != nullptr);

  auto s1 = obj->attributeValue(m_sectr1).toDouble() * M_PI / 180.;
  auto s2 = obj->attributeValue(m_sectr2).toDouble() * M_PI / 180.;

  double leglen;
  bool chartUnits = Settings::instance()->fullLengthLightSectors();
  if (chartUnits) {
    bool ok;
    leglen = obj->attributeValue(m_valnmr).toDouble(&ok);
    if (!ok) {
      leglen = 9. * 1852.;
    }
  } else {
    leglen = 25;
  }

  S57::ElementData e;
  e.count = 5;
  e.offset = 0;
  e.mode = GL_LINE_STRIP_ADJACENCY;

  S57::ElementDataVector elements;
  elements.append(e);

  GL::VertexVector vertices;
  const GLfloat x0 = point->points()[0];
  const GLfloat y0 = point->points()[1];
  const GLfloat x1 = x0 - leglen * sin(s1);
  const GLfloat y1 = y0 - leglen * cos(s1);
  const GLfloat x2 = x0 - leglen * sin(s2);
  const GLfloat y2 = y0 - leglen * cos(s2);

  vertices << 2 * x1 - x0 << 2 * y1 - y0;
  vertices << x1 << y1;
  vertices << x0 << y0;
  vertices << x2 << y2;
  vertices << 2 * x2 - x0 << 2 * y2 - y0;

  auto color = S52::GetColor(m_chblk);
  auto p = new S57::DashedLineLocalData(vertices, elements, color, 1,
                                        as_numeric(S52::LineType::Dashed),
                                        !chartUnits,
                                        QPointF(x0, y0));

  return S57::PaintDataMap{{p->type(), p}};
}


S57::PaintDataMap S52::CSLights05::drawArc(const S57::Object *obj,
                                           float r,
                                           uint lw,
                                           S52::LineType t,
                                           quint32 c) {

  auto point = dynamic_cast<const S57::Geometry::Point*>(obj->geometry());
  Q_ASSERT(point != nullptr);

  auto s1 = obj->attributeValue(m_sectr1).toDouble() * M_PI / 180.;
  auto s2 = obj->attributeValue(m_sectr2).toDouble() * M_PI / 180.;
  while (s2 < s1) s2 += 2 * M_PI;
  const int n0 = qMin(90, qMax(static_cast<int>(r * (s2 - s1)), 2));

  S57::ElementData e;
  e.count = n0 + 2;
  e.offset = 0;
  e.mode = GL_LINE_STRIP_ADJACENCY;

  S57::ElementDataVector elements;
  elements.append(e);

  GL::VertexVector vertices;
  // account adjacency
  vertices << 0 << 0;
  const QPointF p0(point->points()[0], point->points()[1]);
  for (int n = 0; n < n0; n++) {
    const qreal s = s1 + n * (s2 - s1) / (n0 - 1);
    const QPointF p = p0 - r * QPointF(sin(s), cos(s));
    vertices << p.x() << p.y();
  }
  // account adjacency
  int x0 = 2;
  int x1 = 4;
  vertices[0] = 2 * vertices[x0] - vertices[x1];
  vertices[1] = 2 * vertices[x0 + 1] - vertices[x1 + 1];
  x0 = vertices.size() - 4;
  x1 = vertices.size() - 2;
  vertices << 2 * vertices[x1] - vertices[x0];
  vertices << 2 * vertices[x1 + 1] - vertices[x0 + 1];


  auto color = S52::GetColor(c);
  S57::PaintData* p;
  if (t == S52::LineType::Solid) {
    p = new S57::SolidLineLocalData(vertices, elements, color, lw,
                                    true, p0);
  } else {
    p = new S57::DashedLineLocalData(vertices, elements, color, lw,
                                     as_numeric(S52::LineType::Dashed),
                                     true, p0);
  }

  return S57::PaintDataMap{{p->type(), p}};
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

      vals.append(QVariant::fromValue(int(S52::LineType::Solid)));

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
      vals.append(QVariant::fromValue(int(S52::LineType::Dashed)));
      vals.append(QVariant::fromValue(1));
    } else {
      const QVariant catslc = obj->attributeValue(m_catslc);
      const int v2 = catslc.toInt();

      if (catslc.isValid() && (v2 == 6 || v2 == 15 || v2 == 16)) { // Some sort of wharf
        vals.append(QVariant::fromValue(int(S52::LineType::Solid)));
        vals.append(QVariant::fromValue(4));
      } else {
        const QVariant watlev = obj->attributeValue(m_watlev);
        const int v3 = watlev.toInt();

        if (watlev.isValid() && v3 == 2) {
          vals.append(QVariant::fromValue(int(S52::LineType::Solid)));
          vals.append(QVariant::fromValue(2));
        } else if (watlev.isValid() && (v3 == 3 || v3 == 4)) {
          vals.append(QVariant::fromValue(int(S52::LineType::Dashed)));
          vals.append(QVariant::fromValue(2));
        } else {
          vals.append(QVariant::fromValue(int(S52::LineType::Solid)));
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
        //        qDebug() << "TOPMRK01: floating";
        break;
      }
      if (isRigid(it.value())) {
        if (m_rigids.contains(topshp)) {
          topmrk = m_rigids[topshp];
        } else {
          topmrk = m_tmardef1;
        }
        //        qDebug() << "TOPMRK01: rigid";
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

