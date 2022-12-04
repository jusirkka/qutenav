/* -*- coding: utf-8-unix -*-
 *
 * File: src/s52functions.cpp
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
#include "s52functions.h"
#include "s52presentation.h"
#include "s52names.h"
#include "conf_marinerparams.h"
#include <cmath>
#include "types.h"
#include "textmanager.h"
#include "rastersymbolmanager.h"
#include "vectorsymbolmanager.h"
#include "logging.h"
#include <QPainter>
#include <QPaintDevice>
#include "units.h"

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

QStringList S52::AreaColor::descriptions(const QVector<QVariant>&, const S57::Object* obj) const {
  return QStringList {GetClassDescription(obj->classCode())};
}

void S52::AreaColor::paintIcon(QPainter& painter,
                               const QVector<QVariant>& vals, const S57::Object* obj) const {
  auto color = S52::GetColor(vals[0].toUInt());
  color.setAlpha(vals[1].toUInt());
  const auto d = painter.device();
  painter.fillRect(0, 0, d->width(), d->height(), color);
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
    qCWarning(CS57) << "Missing pattern" << S52::GetSymbolInfo(index, S52::SymbolType::Pattern);
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
    //    qCDebug(CS57) << "[VectorPattern:Class]" << S52::GetClassInfo(obj->classCode());
    //    qCDebug(CS57) << "[VectorPattern:Symbol]" << S52::GetSymbolInfo(index, S52::SymbolType::Pattern);
    //    qCDebug(CS57) << "[VectorPattern:Location]" << obj->geometry()->centerLL().print();
    //    for (auto k: obj->attributes().keys()) {
    //      qCDebug(CS57) << GetAttributeInfo(k, obj);
    //    }

    KV::ColorVector colors;
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

QStringList S52::AreaPattern::descriptions(const QVector<QVariant>& vals, const S57::Object* obj) const {
  return QStringList {GetClassDescription(obj->classCode())};
}


void S52::AreaPattern::paintIcon(QPainter& painter,
                                 const QVector<QVariant>& vals, const S57::Object* obj) const {
  quint32 index = vals[0].toUInt();
  // cw direction of rotation counteracts flipped y-coord
  const Angle rot = Angle::fromDegrees(vals[1].toDouble());
  if (!rot.isZero()) {
    VectorSymbolManager::instance()->paintIcon(painter, index, S52::SymbolType::Pattern,
                                               rot.degrees());
  } else {
    const bool ok = RasterSymbolManager::instance()->paintIcon(painter, index, S52::SymbolType::Pattern);
    if (!ok) {
      VectorSymbolManager::instance()->paintIcon(painter, index, S52::SymbolType::Pattern, 0);
    }
  }
}


S57::PaintDataMap S52::LineSimple::execute(const QVector<QVariant>& vals,
                                           const S57::Object* obj) {

  S57::ElementDataVector lines;
  auto line = dynamic_cast<const S57::Geometry::Line*>(obj->geometry());
  if (line == nullptr) {
    auto area = dynamic_cast<const S57::Geometry::Area*>(obj->geometry());
    Q_ASSERT(area != nullptr);
    lines = area->lineElements();
  } else {
    lines = line->elements();
  }

  auto pattern = as_enum<S52::LineType>(vals[0].toUInt(), S52::AllLineTypes);
  auto width = vals[1].toUInt();
  auto color = S52::GetColor(vals[2].toUInt());

  auto p = new S57::LineElemData(lines, 0, color,
                                 S52::LineWidthMM(width), as_numeric(pattern));

  return S57::PaintDataMap{{p->type(), p}};
}

QStringList S52::LineSimple::descriptions(const QVector<QVariant>& vals, const S57::Object* obj) const {
  return QStringList {GetClassDescription(obj->classCode())};
}


void S52::LineSimple::paintIcon(QPainter& painter,
                                const QVector<QVariant>& vals, const S57::Object* obj) const {
  auto pattern = as_enum<S52::LineType>(vals[0].toUInt(), S52::AllLineTypes);
  auto lw = 8; // vals[1].toUInt();
  auto color = S52::GetColor(vals[2].toUInt());

  const auto pen0 = painter.pen();
  const QPen pen1(color, lw, pattern == S52::LineType::Solid ?
                    Qt::SolidLine : pattern == S52::LineType::Dashed ?
                      Qt::DashLine : Qt::DotLine);
  painter.setPen(pen1);
  const auto d = painter.device();
  const int w = d->width();
  const int h = d->height();
  if (obj->geometry()->type() == S57::Geometry::Type::Line) {
    painter.drawLine(0, h / 2, w, h / 2);
  } else {
    const int x0 = lw / 2;
    const int y0 = lw / 2;
    const int x1 = w - lw / 2;
    const int y1 = h - lw / 2;
    painter.drawLine(x0, y0, x1, y0);
    painter.drawLine(x1, y0, x1, y1);
    painter.drawLine(x1, y1, x0, y1);
    painter.drawLine(x0, y1, x0, y0);
  }
  painter.setPen(pen0);
}


S57::PaintDataMap S52::LineComplex::execute(const QVector<QVariant>& vals,
                                            const S57::Object* obj) {

  const quint32 index = vals[0].toUInt();
  const SymbolData s = VectorSymbolManager::instance()->symbolData(index, S52::SymbolType::LineStyle);

  if (!s.isValid()) {
    qCWarning(CS57) << "Missing linestyle" << S52::GetSymbolInfo(index, S52::SymbolType::LineStyle);
    return S57::PaintDataMap();
  }

  S57::ElementDataVector lines;
  auto line = dynamic_cast<const S57::Geometry::Line*>(obj->geometry());
  if (line == nullptr) {
    auto area = dynamic_cast<const S57::Geometry::Area*>(obj->geometry());
    Q_ASSERT(area != nullptr);
    lines = area->lineElements();
  } else {
    lines = line->elements();
  }

  //  qCDebug(CS57) << "[LineStyle:Class]" << S52::GetClassInfo(obj->classCode()) << obj->classCode();
  //  qCDebug(CS57) << "[LineStyle:Symbol]" << S52::GetSymbolInfo(index, S52::SymbolType::LineStyle) << index;
  //  for (auto k: obj->attributes().keys()) {
  //    qCDebug(CS57) << GetAttributeInfo(k, obj);
  //  }

  KV::ColorVector colors;
  for (const S52::Color& c: s.colors()) {
    auto color = S52::GetColor(c.index);
    color.setAlpha(255 - as_numeric(c.alpha) * 255 / 4);
    colors.append(color);
  }
  auto p = new S57::LineStylePaintData(index,
                                       lines,
                                       0,
                                       obj->boundingBox(),
                                       s.advance(),
                                       colors,
                                       s.elements());

  return S57::PaintDataMap{{p->type(), p}};
}

QStringList S52::LineComplex::descriptions(const QVector<QVariant>& vals, const S57::Object* obj) const {
  return QStringList {GetClassDescription(obj->classCode())};
}


void S52::LineComplex::paintIcon(QPainter& painter,
                                 const QVector<QVariant>& vals, const S57::Object* obj) const {
  quint32 index = vals[0].toUInt();
  VectorSymbolManager::instance()->paintIcon(painter, index, S52::SymbolType::LineStyle, 0);
}


S52::PointSymbol::PointSymbol(quint32 index)
  : S52::Function("SY", index)
  , m_catlam(S52::FindIndex("CATLAM"))
  , m_catcam(S52::FindIndex("CATCAM")) {}


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
    qCWarning(CS57) << "Missing symbol" << S52::GetSymbolInfo(index, S52::SymbolType::Single);
    return S57::PaintDataMap();
  }

  QPointF loc = obj->geometry()->center();
  if (obj->geometry()->type() == S57::Geometry::Type::Point) {
    auto pt = dynamic_cast<const S57::Geometry::Point*>(obj->geometry());
    if (pt->points().size() > 2) {
      int i = vals[2].toInt();
      const auto ps = pt->points();
      loc = QPointF(ps[3 * i], ps[3 * i + 1]);
    }
  }

  S57::PaintData* p;
  if (raster) {
    p = new S57::RasterSymbolPaintData(index,
                                       s.offset(),
                                       loc,
                                       s.element());
  } else {

    KV::ColorVector colors;
    for (const S52::Color& c: s.colors()) {
      auto color = S52::GetColor(c.index);
      color.setAlpha(255 - as_numeric(c.alpha) * 255 / 4);
      colors.append(color);
    }

    p = new S57::VectorSymbolPaintData(index,
                                       loc,
                                       rot,
                                       colors,
                                       s.elements());
  }

  return S57::PaintDataMap{{p->type(), p}};
}

QStringList S52::PointSymbol::descriptions(const QVector<QVariant>& vals, const S57::Object* obj) const {
  qCDebug(CS57) << " --- NAME ---" << obj->name() << obj->geometry()->centerLL().print();
  QStringList ds;
  if (obj->attributeValue(m_catlam).isValid()) {
    ds << S52::GetAttributeValueDescription(m_catlam, obj->attributeValue(m_catlam));
  } else if (obj->attributeValue(m_catcam).isValid()) {
    ds << S52::GetAttributeValueDescription(m_catcam, obj->attributeValue(m_catcam));
  } else {
    ds << GetClassDescription(obj->classCode());
  }
  return ds;
}

void S52::PointSymbol::paintIcon(QPainter& painter,
                                 const QVector<QVariant>& vals, const S57::Object* obj) const {
  quint32 index = vals[0].toUInt();
  // cw direction of rotation counteracts flipped y-coord
  const Angle rot = Angle::fromDegrees(vals[1].toDouble());
  if (!rot.isZero()) {
    VectorSymbolManager::instance()->paintIcon(painter, index, S52::SymbolType::Single,
                                               rot.degrees());
  } else {
    const bool ok = RasterSymbolManager::instance()->paintIcon(painter, index, S52::SymbolType::Single);
    if (!ok) {
      VectorSymbolManager::instance()->paintIcon(painter, index, S52::SymbolType::Single, 0);
    }
  }
}


S57::PaintDataMap S52::Text::execute(const QVector<QVariant>& vals,
                                     const S57::Object* obj) {


  const quint8 group = vals[8].toUInt();
  if (!Conf::MarinerParams::TextGrouping().contains(group)) {
    // qCDebug(CS57) << "skipping TX in group" << group;
    return S57::PaintDataMap();
  }

  QString txt;

  const QMetaType::Type t = static_cast<QMetaType::Type>(vals[0].type());
  if (t == QMetaType::UInt) {
    const quint32 index = vals[0].toUInt();
    txt = GetAttributeValueDescription(index, obj->attributeValue(index));
  } else if (t == QMetaType::QString) {
    txt = vals[0].toString();
  } else {
    return S57::PaintDataMap();
  }

  if (txt.isEmpty()) {
    return S57::PaintDataMap();
  }

  // qCDebug(CS57) << "TX:" << as_numeric(obj->geometry()->type()) << txt;


  const QString chars = vals[4].toString();

  Q_ASSERT(chars.left(1).toUInt() == 1); // style: always system's default
  const quint8 width = chars.mid(2, 1).toUInt();
  Q_ASSERT(width == 1 || width == 2); // always roman slant

  auto weight = as_enum<TXT::Weight>(chars.mid(1, 1).toUInt(), TXT::AllWeights);

  auto space = as_enum<TXT::Space>(vals[3].toUInt(), TXT::AllSpaces);
  if (space != TXT::Space::Standard && space != TXT::Space::Wrapped) {
    qCWarning(CS57) << "TX: text spacing type" << as_numeric(space) << "not implemented";
    return S57::PaintDataMap();
  }
  // TODO: we do not actually wrap although TXT::Space::Wrapped is accepted
  if (txt.length() > 80) {
    qCWarning(CS57) << "Cutting long text" << txt;
    txt = txt.mid(80);
  }

  auto hjust = as_enum<TXT::HJust>(vals[1].toUInt(), TXT::AllHjusts);
  auto vjust = as_enum<TXT::VJust>(vals[2].toUInt(), TXT::AllVjusts);
  auto bodySize = chars.mid(3).toUInt();
  auto offsetX = vals[5].toInt();
  auto offsetY = vals[6].toInt();

  const int ticket = TextManager::instance()->ticket(txt,
                                                     weight,
                                                     hjust,
                                                     vjust,
                                                     bodySize,
                                                     offsetX,
                                                     offsetY);

  QPointF loc = obj->geometry()->center();
  if (obj->geometry()->type() == S57::Geometry::Type::Point) {
    auto pt = dynamic_cast<const S57::Geometry::Point*>(obj->geometry());
    if (pt->points().size() > 2) {
      int i = vals[9].toInt();
      const auto ps = pt->points();
      loc = QPointF(ps[3 * i], ps[3 * i + 1]);
    }
  }


  const QColor color = S52::GetColor(vals[7].toUInt());

  S57::PaintData* p = new S57::TextElemData(loc,
                                            ticket,
                                            color);

  return S57::PaintDataMap{{p->type(), p}};
}

QStringList S52::Text::descriptions(const QVector<QVariant>& vals, const S57::Object* obj) const {

  QString txt;

  const QMetaType::Type t = static_cast<QMetaType::Type>(vals[0].type());
  if (t == QMetaType::UInt) {
    const quint32 index = vals[0].toUInt();
    txt = GetAttributeValueDescription(index, obj->attributeValue(index));
  } else if (t == QMetaType::QString) {
    txt = vals[0].toString();
  } else {
    return QStringList();
  }

  if (txt.isEmpty() || txt.length() > 80) {//
    return QStringList();
  }

  return QStringList {"\"" + txt + "\""};
}


S57::PaintDataMap S52::TextExtended::execute(const QVector<QVariant>& vals,
                                             const S57::Object* obj) {

  const int numAttrs = vals[1].toInt();

  const quint8 group = vals[9 + numAttrs].toUInt();
  if (!Conf::MarinerParams::TextGrouping().contains(group)) {
    // qCDebug(CS57) << "skipping TX in group" << group;
    return S57::PaintDataMap();
  }

  const QString txt = vals[0].toString();
  if (txt.isEmpty()) {
    return S57::PaintDataMap();
  }

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
    case QMetaType::UInt:
      sprintf(buf, format, vals[2 + i].toInt());
      break;
    case QMetaType::UnknownType: // Empty QVariant
      return S57::PaintDataMap();
    default:
      qCWarning(CS57) << "TE: Unhandled attribute value" << vals[2 + i];
      //      qCWarning(CS57) << "[Class]" << S52::GetClassInfo(obj->classCode());
      //      qCWarning(CS57) << "[Location]" << obj->geometry()->centerLL().print();
      //      for (auto k: obj->attributes().keys()) {
      //        qCWarning(CS57) << GetAttributeInfo(k, obj);
      //      }
      return S57::PaintDataMap();
    }
    strcpy(format, buf);
  }
  // qCDebug(CS57) << "TE:" << as_numeric(obj->geometry()->type()) << buf;

  QVector<QVariant> txVals;
  txVals.append(QVariant::fromValue(QString::fromUtf8(buf)));
  for (int i = 0; i < 8; i++) {
    txVals.append(vals[numAttrs + 2 + i]);
  }

  return S52::FindFunction("TX")->execute(txVals, obj);
}

QStringList S52::TextExtended::descriptions(const QVector<QVariant>& vals, const S57::Object* obj) const {

  const int numAttrs = vals[1].toInt();

  const QString txt = vals[0].toString();
  if (txt.isEmpty()) {
    return QStringList();
  }

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
    case QMetaType::UInt:
      sprintf(buf, format, vals[2 + i].toInt());
      break;
    case QMetaType::UnknownType: // Empty QVariant
      return QStringList();
    default:
      qCWarning(CS57) << "TE: Unhandled attribute value" << vals[2 + i];
      return QStringList();
    }
    strcpy(format, buf);
  }

  QVector<QVariant> txVals;
  txVals.append(QVariant::fromValue(QString::fromUtf8(buf)));
  for (int i = 0; i < 8; i++) {
    txVals.append(vals[numAttrs + 2 + i]);
  }

  return S52::FindFunction("TX")->descriptions(txVals, obj);
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

  // Determine the color based on mariner selections

  quint32 sym = m_depit;
  double depth = -1.;

  if (obj->attributeValue(m_drval1).isValid()) {
    sym = m_depvs;
    depth = obj->attributeValue(m_drval1).toDouble();
    if (obj->attributeValue(m_drval2).isValid()) {
      depth = qMin(depth, obj->attributeValue(m_drval2).toDouble());
    }
  }

  const qreal safetyC = Conf::MarinerParams::SafetyContour();
  const qreal shallowC = Conf::MarinerParams::ShallowContour();
  const qreal deepC = Conf::MarinerParams::DeepContour();
  const bool twoShades = Conf::MarinerParams::TwoShades();

  if (twoShades && depth >= safetyC) {
    sym = m_depdw;
  } else {
    if (depth >= shallowC) {
      sym = m_depms;
    }
    if (depth >= safetyC) {
      sym = m_depmd;
    }
    if (depth >= deepC) {
      sym = m_depdw;
    }
  }

  if (obj->classCode() != m_drgare) {
    const QVector<QVariant> v0 {sym, 255};
    return S52::FindFunction("AC")->execute(v0, obj);
  }

  if (!obj->attributeValue(m_drval1).isValid()) {
    sym = m_depmd;
  }

  const QVector<QVariant> v0 {sym, 255};
  S57::PaintDataMap ps = S52::FindFunction("AC")->execute(v0, obj);

  // run AP(DRGARE01);LS(DASH,1,CHGRF)
  const QVector<QVariant> v1 {m_drgare01, 0.};
  ps += S52::FindFunction("AP")->execute(v1, obj);

  const QVector<QVariant> v2 {as_numeric(S52::LineType::Dashed), 1, m_chgrf};
  ps += S52::FindFunction("LS")->execute(v2, obj);

  return ps;
}

QStringList S52::CSDepthArea01::descriptions(const QVector<QVariant>& vals, const S57::Object* obj) const {
  const QString lower = obj->attributeValue(m_drval1).isValid() ?
        QString::number(obj->attributeValue(m_drval1).toDouble()) : "";
  const QString upper = obj->attributeValue(m_drval2).isValid() ?
        QString::number(obj->attributeValue(m_drval2).toDouble()) : "";

  return QStringList {QString("(%1-%2m)").arg(lower).arg(upper)};
}


void S52::CSDepthArea01::paintIcon(QPainter& painter,
                                   const QVector<QVariant>& /*vals*/, const S57::Object* obj) const {

  // Determine the color based on mariner selections

  quint32 sym = m_depit;
  double depth = -1.;

  if (obj->attributeValue(m_drval1).isValid()) {
    sym = m_depvs;
    depth = obj->attributeValue(m_drval1).toDouble();
    if (obj->attributeValue(m_drval2).isValid()) {
      depth = qMin(depth, obj->attributeValue(m_drval2).toDouble());
    }
  }

  const qreal safetyC = Conf::MarinerParams::SafetyContour();
  const qreal shallowC = Conf::MarinerParams::ShallowContour();
  const qreal deepC = Conf::MarinerParams::DeepContour();
  const bool twoShades = Conf::MarinerParams::TwoShades();

  if (twoShades && depth >= safetyC) {
    sym = m_depdw;
  } else {
    if (depth >= shallowC) {
      sym = m_depms;
    }
    if (depth >= safetyC) {
      sym = m_depmd;
    }
    if (depth >= deepC) {
      sym = m_depdw;
    }
  }

  if (obj->classCode() != m_drgare) {
    const QVector<QVariant> v0 {sym, 255};
    S52::FindFunction("AC")->paintIcon(painter, v0, obj);
  }

  if (!obj->attributeValue(m_drval1).isValid()) {
    sym = m_depmd;
  }

  const QVector<QVariant> v0 {sym, 255};
  S52::FindFunction("AC")->paintIcon(painter, v0, obj);

  // run AP(DRGARE01);LS(DASH,1,CHGRF)
  const QVector<QVariant> v1 {m_drgare01, 0.};
  S52::FindFunction("AP")->paintIcon(painter, v1, obj);

  const QVector<QVariant> v2 {as_numeric(S52::LineType::Dashed), 1, m_chgrf};
  S52::FindFunction("LS")->paintIcon(painter, v2, obj);
}

S52::CSResArea02::CSResArea02(quint32 index)
  : Function("RESARE02", index)
  , m_chmgd(FindIndex("CHMGD"))
  , m_catrea(FindIndex("CATREA"))
  , m_restrn(FindIndex("RESTRN"))
  , m_entres51(FindIndex("ENTRES51"))
  , m_entres61(FindIndex("ENTRES61"))
  , m_entres71(FindIndex("ENTRES71"))
  , m_achres51(FindIndex("ACHRES51"))
  , m_achres61(FindIndex("ACHRES61"))
  , m_achres71(FindIndex("ACHRES71"))
  , m_fshres51(FindIndex("FSHRES51"))
  , m_fshres61(FindIndex("FSHRES61"))
  , m_fshres71(FindIndex("FSHRES71"))
  , m_infare51(FindIndex("INFARE51"))
  , m_rsrdef51(FindIndex("RSRDEF51"))
  , m_ctyare51(FindIndex("CTYARE51"))
  , m_ctyare71(FindIndex("CTYARE71"))
  , m_anchor_set {1, 2}
  , m_entry_set {7, 6, 14}
  , m_fish_set {3, 4, 5, 6}
  , m_sport_set {9, 10, 11, 12, 13}
  , m_mil_set {1, 8, 9, 12, 14, 19, 21, 25}
  , m_nat_set {4, 5, 6, 7, 10, 18, 20, 22, 23, 24}

{}

S57::PaintDataMap S52::CSResArea02::execute(const QVector<QVariant>&,
                                            const S57::Object* obj) {
  // qCDebug(CS57) << "[CSResArea02:Class]" << S52::GetClassInfo(obj->classCode());
  // for (auto k: obj->attributes().keys()) {
  //   qCDebug(CS57) << GetAttributeInfo(k, obj);
  // }

  S57::PaintDataMap ps;

  QSet<int> catrea = obj->attributeSetValue(m_catrea);
  QSet<int> restrn = obj->attributeSetValue(m_restrn);
  quint32 sym;

  if (obj->attributeValue(m_restrn).isValid()) {
    if (restrn.intersects(m_entry_set)) {
      // continuation A
      if (restrn.intersects(m_anchor_set) ||
          restrn.intersects(m_fish_set) ||
          catrea.intersects(m_mil_set)) {
        sym = m_entres61;
      } else if (restrn.intersects(m_sport_set) ||
                 catrea.intersects(m_nat_set)) {
        sym = m_entres71;
      } else {
        sym = m_entres51;
      }

      const QVector<QVariant> v0 {sym, 0.};
      ps += S52::FindFunction("SY")->execute(v0, obj);

      if (Conf::MarinerParams::PlainBoundaries()) {
        const QVector<QVariant> vals {as_numeric(S52::LineType::Dashed), 2, m_chmgd};
        ps += S52::FindFunction("LS")->execute(vals, obj);
      } else {
        const QVector<QVariant> vals {m_ctyare51};
        ps += S52::FindFunction("LC")->execute(vals, obj);
      }

      auto p = new S57::PriorityData(6);
      ps.insert(p->type(), p);

      return ps;
    }
    if (restrn.intersects(m_anchor_set)) {
      // continuation B
      if (restrn.intersects(m_fish_set) ||
          catrea.intersects(m_mil_set)) {
        sym = m_achres61;
      } else if (restrn.intersects(m_sport_set) ||
                 catrea.intersects(m_nat_set)) {
        sym = m_achres71;
      } else {
        sym = m_achres51;
      }

      const QVector<QVariant> v0 {sym, 0.};
      ps += S52::FindFunction("SY")->execute(v0, obj);

      if (Conf::MarinerParams::PlainBoundaries()) {
        const QVector<QVariant> vals {as_numeric(S52::LineType::Dashed), 2, m_chmgd};
        ps += S52::FindFunction("LS")->execute(vals, obj);
      } else {
        const QVector<QVariant> vals {m_achres51};
        ps += S52::FindFunction("LC")->execute(vals, obj);
      }

      auto p = new S57::PriorityData(6);
      ps.insert(p->type(), p);

      return ps;
    }

    if (restrn.intersects(m_fish_set)) {
      // continuation C
      if (catrea.intersects(m_mil_set)) {
        sym = m_fshres61;
      } else if (restrn.intersects(m_sport_set) ||
                 catrea.intersects(m_nat_set))
      {
        sym = m_fshres71;
      } else {
        sym = m_fshres51;
      }

      const QVector<QVariant> v0 {sym, 0.};
      ps += S52::FindFunction("SY")->execute(v0, obj);

      if (Conf::MarinerParams::PlainBoundaries()) {
        const QVector<QVariant> vals {as_numeric(S52::LineType::Dashed), 2, m_chmgd};
        ps += S52::FindFunction("LS")->execute(vals, obj);
      } else {
        const QVector<QVariant> vals {m_fshres51};
        ps += S52::FindFunction("LC")->execute(vals, obj);
      }

      auto p = new S57::PriorityData(6);
      ps.insert(p->type(), p);
      return ps;
    }

    if (restrn.intersects(m_sport_set)) {
      sym = m_infare51;
    } else {
      sym = m_rsrdef51;
    }

    const QVector<QVariant> v0 {sym, 0.};
    ps += S52::FindFunction("SY")->execute(v0, obj);

    if (Conf::MarinerParams::PlainBoundaries()) {
      const QVector<QVariant> vals {as_numeric(S52::LineType::Dashed), 2, m_chmgd};
      ps += S52::FindFunction("LS")->execute(vals, obj);
    } else {
      const QVector<QVariant> vals {m_ctyare51};
      ps += S52::FindFunction("LC")->execute(vals, obj);
    }

    return ps;
  }

  // continuation D
  sym = m_rsrdef51;
  if (catrea.intersects(m_mil_set) &&
      catrea.intersects(m_nat_set)) {
    sym = m_ctyare71;
  } else if (catrea.intersects(m_mil_set)) {
    sym = m_ctyare51;
  } else if (catrea.intersects(m_nat_set)) {
    sym = m_infare51;
  }

  const QVector<QVariant> v0 {sym, 0.};
  ps += S52::FindFunction("SY")->execute(v0, obj);

  if (Conf::MarinerParams::PlainBoundaries()) {
    const QVector<QVariant> vals {as_numeric(S52::LineType::Dashed), 2, m_chmgd};
    ps += S52::FindFunction("LS")->execute(vals, obj);
  } else {
    const QVector<QVariant> vals {m_ctyare51};
    ps += S52::FindFunction("LC")->execute(vals, obj);
  }

  return ps;

}


QStringList S52::CSResArea02::descriptions(const QVector<QVariant>& vals, const S57::Object* obj) const {
  QStringList ds;
  if (obj->attributeValue(m_catrea).isValid()) {
    ds << S52::GetAttributeValueDescription(m_catrea, obj->attributeValue(m_catrea));
  }
  if (obj->attributeValue(m_restrn).isValid()) {
    ds << S52::GetAttributeValueDescription(m_restrn, obj->attributeValue(m_restrn));
  }
  return ds;
}

void S52::CSResArea02::paintIcon(QPainter& painter,
                                 const QVector<QVariant>& /*vals*/, const S57::Object* obj) const {
  QSet<int> catrea = obj->attributeSetValue(m_catrea);
  QSet<int> restrn = obj->attributeSetValue(m_restrn);
  quint32 sym;

  if (obj->attributeValue(m_restrn).isValid()) {
    if (restrn.intersects(m_entry_set)) {
      // continuation A
      if (restrn.intersects(m_anchor_set) ||
          restrn.intersects(m_fish_set) ||
          catrea.intersects(m_mil_set)) {
        sym = m_entres61;
      } else if (restrn.intersects(m_sport_set) ||
                 catrea.intersects(m_nat_set)) {
        sym = m_entres71;
      } else {
        sym = m_entres51;
      }

      const QVector<QVariant> v0 {sym, 0.};
      S52::FindFunction("SY")->paintIcon(painter, v0, obj);

      if (Conf::MarinerParams::PlainBoundaries()) {
        const QVector<QVariant> vals {as_numeric(S52::LineType::Dashed), 2, m_chmgd};
        S52::FindFunction("LS")->paintIcon(painter, vals, obj);
      } else {
        const QVector<QVariant> vals {m_ctyare51};
        S52::FindFunction("LC")->paintIcon(painter, vals, obj);
      }

      return;
    }

    if (restrn.intersects(m_anchor_set)) {
      // continuation B
      if (restrn.intersects(m_fish_set) ||
          catrea.intersects(m_mil_set)) {
        sym = m_achres61;
      } else if (restrn.intersects(m_sport_set) ||
                 catrea.intersects(m_nat_set)) {
        sym = m_achres71;
      } else {
        sym = m_achres51;
      }

      const QVector<QVariant> v0 {sym, 0.};
      S52::FindFunction("SY")->paintIcon(painter, v0, obj);

      if (Conf::MarinerParams::PlainBoundaries()) {
        const QVector<QVariant> vals {as_numeric(S52::LineType::Dashed), 2, m_chmgd};
        S52::FindFunction("LS")->paintIcon(painter, vals, obj);
      } else {
        const QVector<QVariant> vals {m_achres51};
        S52::FindFunction("LC")->paintIcon(painter, vals, obj);
      }

      return;
    }

    if (restrn.intersects(m_fish_set)) {
      // continuation C
      if (catrea.intersects(m_mil_set)) {
        sym = m_fshres61;
      } else if (restrn.intersects(m_sport_set) ||
                 catrea.intersects(m_nat_set))
      {
        sym = m_fshres71;
      } else {
        sym = m_fshres51;
      }

      const QVector<QVariant> v0 {sym, 0.};
      S52::FindFunction("SY")->paintIcon(painter, v0, obj);

      if (Conf::MarinerParams::PlainBoundaries()) {
        const QVector<QVariant> vals {as_numeric(S52::LineType::Dashed), 2, m_chmgd};
        S52::FindFunction("LS")->paintIcon(painter, vals, obj);
      } else {
        const QVector<QVariant> vals {m_fshres51};
        S52::FindFunction("LC")->paintIcon(painter, vals, obj);
      }

      return;
    }

    if (restrn.intersects(m_sport_set)) {
      sym = m_infare51;
    } else {
      sym = m_rsrdef51;
    }

    const QVector<QVariant> v0 {sym, 0.};
    S52::FindFunction("SY")->paintIcon(painter, v0, obj);

    if (Conf::MarinerParams::PlainBoundaries()) {
      const QVector<QVariant> vals {as_numeric(S52::LineType::Dashed), 2, m_chmgd};
      S52::FindFunction("LS")->paintIcon(painter, vals, obj);
    } else {
      const QVector<QVariant> vals {m_ctyare51};
      S52::FindFunction("LC")->paintIcon(painter, vals, obj);
    }

    return;
  }

  // continuation D
  sym = m_rsrdef51;
  if (catrea.intersects(m_mil_set) &&
      catrea.intersects(m_nat_set)) {
    sym = m_ctyare71;
  } else if (catrea.intersects(m_mil_set)) {
    sym = m_ctyare51;
  } else if (catrea.intersects(m_nat_set)) {
    sym = m_infare51;
  }

  const QVector<QVariant> v0 {sym, 0.};
  S52::FindFunction("SY")->paintIcon(painter, v0, obj);

  if (Conf::MarinerParams::PlainBoundaries()) {
    const QVector<QVariant> vals {as_numeric(S52::LineType::Dashed), 2, m_chmgd};
    S52::FindFunction("LS")->paintIcon(painter, vals, obj);
  } else {
    const QVector<QVariant> vals {m_ctyare51};
    S52::FindFunction("LC")->paintIcon(painter, vals, obj);
  }

}


S52::CSDataCov01::CSDataCov01(quint32 index)
  : Function("DATCVR01", index)
  , m_hodata01(S52::FindIndex("HODATA01")) {}

S57::PaintDataMap S52::CSDataCov01::execute(const QVector<QVariant>&,
                                            const S57::Object* obj) {
  QVector<QVariant> vals {m_hodata01};
  return S52::FindFunction("LC")->execute(vals, obj);
}

S57::PaintDataMap S52::CSDepthArea02::execute(const QVector<QVariant>& vals,
                                              const S57::Object* obj) {
  return S52::FindFunction("DEPARE01")->execute(vals, obj);
}

QStringList S52::CSDepthArea02::descriptions(const QVector<QVariant>& vals,
                                             const S57::Object* obj) const {
  return S52::FindFunction("DEPARE01")->descriptions(vals, obj);
}

void S52::CSDepthArea02::paintIcon(QPainter& painter,
                                   const QVector<QVariant>& vals, const S57::Object* obj) const {
  return S52::FindFunction("DEPARE01")->paintIcon(painter, vals, obj);
}

S52::CSDepthContours02::CSDepthContours02(quint32 index)
  : Function("DEPCNT02", index)
  , m_depare(FindIndex("DEPARE"))
  , m_drval1(FindIndex("DRVAL1"))
  , m_drval2(FindIndex("DRVAL2"))
  , m_valdco(FindIndex("VALDCO"))
  , m_quapos(FindIndex("QUAPOS"))
  , m_depsc(FindIndex("DEPSC"))
  , m_depcn(FindIndex("DEPCN"))
{}

S57::PaintDataMap S52::CSDepthContours02::execute(const QVector<QVariant>&,
                                                  const S57::Object* obj) {
  bool isSafetyContour = false;
  const double sc = Conf::MarinerParams::SafetyContour();
  if (obj->classCode() == m_depare && obj->geometry()->type() == S57::Geometry::Type::Line) {
    const double d1 = obj->attributeValue(m_drval1).isValid() ? obj->attributeValue(m_drval1).toDouble() : 0.;
    const double d2 = obj->attributeValue(m_drval2).isValid() ? obj->attributeValue(m_drval2).toDouble() : 0.;
    isSafetyContour = d1 <= sc && sc <= d2;
  } else {
    // continuation A
    const double v0 = obj->attributeValue(m_valdco).isValid() ? obj->attributeValue(m_valdco).toDouble() : 0.;
    isSafetyContour = v0 == obj->getSafetyContour(sc);
  }
  // continuation B
  S57::PaintDataMap ps;
  const int quapos = obj->attributeValue(m_quapos).isValid() ? obj->attributeValue(m_quapos).toInt() : 0;
  if (quapos > 1 && quapos < 10) {
    if (isSafetyContour) {
      const QVector<QVariant> vals {as_numeric(S52::LineType::Dashed), 2, m_depsc};
      ps += S52::FindFunction("LS")->execute(vals, obj);
      auto p = new S57::OverrideData(true);
      ps.insert(p->type(), p);
    } else {
      const QVector<QVariant> vals {as_numeric(S52::LineType::Dashed), 1, m_depcn};
      ps += S52::FindFunction("LS")->execute(vals, obj);
    }
  } else {
    if (isSafetyContour) {
      const QVector<QVariant> vals {as_numeric(S52::LineType::Solid), 2, m_depsc};
      ps += S52::FindFunction("LS")->execute(vals, obj);
      auto p = new S57::OverrideData(true);
      ps.insert(p->type(), p);
    } else {
      const QVector<QVariant> vals {as_numeric(S52::LineType::Solid), 1, m_depcn};
      ps += S52::FindFunction("LS")->execute(vals, obj);
    }
  }
  return ps;
}

QStringList S52::CSDepthContours02::descriptions(const QVector<QVariant>& vals,
                                                 const S57::Object* obj) const {

  if (obj->classCode() == m_depare && obj->geometry()->type() == S57::Geometry::Type::Line) {
    const QString lower = obj->attributeValue(m_drval1).isValid() ?
          QString::number(obj->attributeValue(m_drval1).toDouble()) : "";
    const QString upper = obj->attributeValue(m_drval2).isValid() ?
          QString::number(obj->attributeValue(m_drval2).toDouble()) : "";
    return QStringList {QString("(%1-%2m)").arg(lower).arg(upper)};
  }

  const QString val = obj->attributeValue(m_valdco).isValid() ?
        QString::number(obj->attributeValue(m_valdco).toDouble()) : "";
  if (!val.isEmpty()) {
    return QStringList {QString("(%1m)").arg(val)};
  }

  return QStringList();
}

void S52::CSDepthContours02::paintIcon(QPainter& painter, const QVector<QVariant>&,
                                       const S57::Object* obj) const {
  bool isSafetyContour = false;
  const double sc = Conf::MarinerParams::SafetyContour();
  if (obj->classCode() == m_depare && obj->geometry()->type() == S57::Geometry::Type::Line) {
    const double d1 = obj->attributeValue(m_drval1).isValid() ? obj->attributeValue(m_drval1).toDouble() : 0.;
    const double d2 = obj->attributeValue(m_drval2).isValid() ? obj->attributeValue(m_drval2).toDouble() : 0.;
    isSafetyContour = d1 <= sc && sc <= d2;
  } else {
    // continuation A
    const double v0 = obj->attributeValue(m_valdco).isValid() ? obj->attributeValue(m_valdco).toDouble() : 0.;
    isSafetyContour = v0 == obj->getSafetyContour(sc);
  }
  // continuation B
  const int quapos = obj->attributeValue(m_quapos).isValid() ? obj->attributeValue(m_quapos).toInt() : 0;
  if (quapos > 1 && quapos < 10) {
    if (isSafetyContour) {
      const QVector<QVariant> vals {as_numeric(S52::LineType::Dashed), 2, m_depsc};
      S52::FindFunction("LS")->paintIcon(painter, vals, obj);
    } else {
      const QVector<QVariant> vals {as_numeric(S52::LineType::Dashed), 1, m_depcn};
      S52::FindFunction("LS")->paintIcon(painter, vals, obj);
    }
  } else {
    if (isSafetyContour) {
      const QVector<QVariant> vals {as_numeric(S52::LineType::Solid), 2, m_depsc};
      S52::FindFunction("LS")->paintIcon(painter, vals, obj);
    } else {
      const QVector<QVariant> vals {as_numeric(S52::LineType::Solid), 1, m_depcn};
      S52::FindFunction("LS")->paintIcon(painter, vals, obj);
    }
  }
}



S52::CSLights05::CSLights05(quint32 index)
  : Function("LIGHTS05", index)
  , m_catlit(S52::FindIndex("CATLIT"))
  , m_lights(S52::FindIndex("LIGHTS"))
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
  //  qCDebug(CS57) << "[CSLights05:Class]" << S52::GetClassInfo(obj->classCode()) << obj->classCode();
  //  for (auto k: obj->attributes().keys()) {
  //    qCDebug(CS57) << GetAttributeInfo(k, obj);
  //  }

  auto catlit = obj->attributeSetValue(m_catlit);

  if (!catlit.isEmpty()) {
    if (catlit.contains(floodlight) || catlit.contains(spotlight)) {
      const QVector<QVariant> vals {m_lights82, 0.};
      return S52::FindFunction("SY")->execute(vals, obj);
    }
    if (catlit.contains(striplight)) {
      const QVector<QVariant> vals {m_lights81, 0.};
      return S52::FindFunction("SY")->execute(vals, obj);
    }
  }

  auto cols = obj->attributeSetValue(m_colour);
  if (cols.isEmpty()) {
    cols.insert(magenta);
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
    if (cols.intersects(m_set_wyo)) {
      S57::Object::LocationIterator it = obj->others();
      for (; it != obj->othersEnd() && it.key() == obj->geometry()->centerLL(); ++it) {
        if (it.value() == obj) continue;
        const S57::Object* other = it.value();
        if (other->classCode() != m_lights) continue;
        flare_at_45 = true;
        break;
      }
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
        vals.append(QVariant::fromValue(0.));
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
      if (s1 < smin) smin = s1;
      continue;
    }
    auto v1 = it.value()->attributeValue(m_sectr1);
    auto v2 = it.value()->attributeValue(m_sectr2);
    if (overlaps_and_smaller(s1, s2, v1, v2, smin)) {
      // qCDebug(CS57) << "Extended radius";
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
  auto litvis = obj->attributeSetValue(m_litvis);
  if (litvis.intersects(m_set_faint)) {
    ps += drawArc(obj, arc_radius, 2, S52::LineType::Dashed, m_chblk);
  } else {
    ps += drawArc(obj, arc_radius - S52::LineWidthMM(2.25), 1.5, S52::LineType::Solid, m_outlw);
    ps += drawArc(obj, arc_radius, 3, S52::LineType::Solid, sectorColor);
    ps += drawArc(obj, arc_radius + S52::LineWidthMM(2.25), 1.5, S52::LineType::Solid, m_outlw);
  }

  return ps;
}

QStringList S52::CSLights05::descriptions(const QVector<QVariant>& vals,
                                          const S57::Object* obj) const {
  QStringList ds {GetClassDescription(obj->classCode())};
  if (obj->attributeValue(m_catlit).isValid()) {
    ds << S52::GetAttributeValueDescription(m_catlit, obj->attributeValue(m_catlit));
    auto catlit = obj->attributeSetValue(m_catlit);
    if (catlit.contains(directional) || catlit.contains(moire_effect)) {
      auto orient = obj->attributeValue(m_orient);
      if (orient.isValid()) {
        ds << QString("%1°").arg(orient.toDouble(), 3, 'f', 0, '0');
      }
    }
  }
  const bool isSector = obj->attributeValue(m_sectr1).isValid() && obj->attributeValue(m_sectr2).isValid();
  if (isSector) {
    auto s1 = obj->attributeValue(m_sectr1).toFloat();
    float smin = 360;
    S57::Object::LocationIterator it = obj->others();
    for (; it != obj->othersEnd() && it.key() == obj->geometry()->centerLL(); ++it) {
      if (it.value() == obj) {
        if (s1 < smin) smin = s1;
        continue;
      }
      if (!it.value()->attributeValue(m_sectr1).isValid()) continue;
      const auto s3 = it.value()->attributeValue(m_sectr1).toFloat();
      if (s3 < smin) smin = s3;
    }
    // show text for only one sector
    if (s1 != smin) return QStringList();
  }

  const QString desc = litdsn01(obj);
  if (!desc.isEmpty()) {
    ds << desc;
  }
  return ds;
}

void S52::CSLights05::paintIcon(QPainter& painter,
                                const QVector<QVariant>& /*vals*/, const S57::Object* obj) const {
  auto catlit = obj->attributeSetValue(m_catlit);

  if (!catlit.isEmpty()) {
    if (catlit.contains(floodlight) || catlit.contains(spotlight)) {
      const QVector<QVariant> vals {m_lights82, 0.};
      S52::FindFunction("SY")->paintIcon(painter, vals, obj);
      return;
    }
    if (catlit.contains(striplight)) {
      const QVector<QVariant> vals {m_lights81, 0.};
      S52::FindFunction("SY")->paintIcon(painter, vals, obj);
      return;
    }
  }

  auto cols = obj->attributeSetValue(m_colour);
  if (cols.isEmpty()) {
    cols.insert(magenta);
  }

  quint32 lightSymbol;
  if (cols == m_set_wr || cols == m_set_r) {
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
    if (cols.intersects(m_set_wyo)) {
      S57::Object::LocationIterator it = obj->others();
      for (; it != obj->othersEnd() && it.key() == obj->geometry()->centerLL(); ++it) {
        if (it.value() == obj) continue;
        const S57::Object* other = it.value();
        if (other->classCode() != m_lights) continue;
        flare_at_45 = true;
        break;
      }
    }

    QVector<QVariant> vals;
    vals.append(QVariant::fromValue(lightSymbol));

    if (catlit.contains(directional) || catlit.contains(moire_effect)) {
      auto orient = obj->attributeValue(m_orient);
      if (orient.isValid()) {
        vals.append(orient);
        S52::FindFunction("SY")->paintIcon(painter, vals, obj);
      } else {
        vals.clear();
        vals.append(QVariant::fromValue(m_quesmrk1));
        vals.append(QVariant::fromValue(0.));
        S52::FindFunction("SY")->paintIcon(painter, vals, obj);
      }
    } else {
      vals.append(QVariant::fromValue(flare_at_45 ? 45. : 135.));
      S52::FindFunction("SY")->paintIcon(painter, vals, obj);
    }
    return;
  }

  // Continuation B
  auto s1 = obj->attributeValue(m_sectr1).toFloat();
  auto s2 = obj->attributeValue(m_sectr2).toFloat();
  while (s2 < s1) s2 += 360.;
  const bool allRound = s2 - s1 < 1. || std::abs(s2 - s1 - 360.) < 1.;

  if (allRound) {
    QVector<QVariant> vals {lightSymbol, 135.};
    S52::FindFunction("SY")->paintIcon(painter, vals, obj);
    return;
  }

  // TODO: draw some symbol for sector light
}



static void maxbox(QPointF& ll, QPointF& ur, qreal x, qreal y) {
  ll.setX(qMin(ll.x(), x));
  ll.setY(qMin(ll.y(), y));
  ur.setX(qMax(ur.x(), x));
  ur.setY(qMax(ur.y(), y));
}

S57::PaintDataMap S52::CSLights05::drawDirection(const S57::Object *obj) const {
  auto point = dynamic_cast<const S57::Geometry::Point*>(obj->geometry());
  Q_ASSERT(point != nullptr);

  if (!obj->attributeValue(m_orient).isValid()) {
    qCDebug(CS57) << "CSLights05::drawDirection: Orient not present";
    return S57::PaintDataMap();
  }
  const double orient = obj->attributeValue(m_orient).toDouble() * M_PI / 180.;

  double valnmr;
  if (obj->attributeValue(m_valnmr).isValid()) {
    valnmr = obj->attributeValue(m_valnmr).toDouble() * 1852.;
  } else {
    valnmr = 9. * 1852.;
  }

  S57::ElementData e;
  e.count = 4;
  e.offset = 0;
  e.mode = GL_LINE_STRIP_ADJACENCY_EXT;

  GL::VertexVector vertices;
  const GLfloat x0 = point->points()[0];
  const GLfloat y0 = point->points()[1];
  const GLfloat x1 = x0 - valnmr * sin(orient);
  const GLfloat y1 = y0 - valnmr * cos(orient);

  vertices << 2 * x0 - x1 << 2 * y0 - y1;
  vertices << x0 << y0;
  vertices << x1 << y1;
  vertices << 2 * x1 - x0 << 2 * y1 - y0;

  // bbox
  QPointF ll(1.e15, 1.e15);
  QPointF ur(-1.e15, -1.e15);
  maxbox(ll, ur, x0, y0);
  maxbox(ll, ur, x1, y1);
  e.bbox = QRectF(ll, ur);

  S57::ElementDataVector elements;
  elements.append(e);

  auto color = S52::GetColor(m_chblk);
  auto p = new S57::LineLocalData(vertices, elements, color, S52::LineWidthMM(1),
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
  bool chartUnits = Conf::MarinerParams::FullLengthSectors();
  if (chartUnits) {
    if (obj->attributeValue(m_valnmr).isValid()) {
      leglen = obj->attributeValue(m_valnmr).toDouble() * 1852.;
    } else {
      leglen = 9. * 1852.;
    }
  } else {
    leglen = 25;
  }

  S57::ElementData e;
  e.count = 5;
  e.offset = 0;
  e.mode = GL_LINE_STRIP_ADJACENCY_EXT;

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

  // bbox
  QPointF ll(1.e15, 1.e15);
  QPointF ur(-1.e15, -1.e15);
  maxbox(ll, ur, x0, y0);
  maxbox(ll, ur, x1, y1);
  maxbox(ll, ur, x2, y2);
  e.bbox = QRectF(ll, ur);

  S57::ElementDataVector elements;
  elements.append(e);


  auto color = S52::GetColor(m_chblk);
  auto p = new S57::LineLocalData(vertices, elements, color, S52::LineWidthMM(1),
                                  as_numeric(S52::LineType::Dashed),
                                  !chartUnits,
                                  QPointF(x0, y0));

  return S57::PaintDataMap{{p->type(), p}};
}


S57::PaintDataMap S52::CSLights05::drawArc(const S57::Object *obj,
                                           float r,
                                           float lw,
                                           S52::LineType t,
                                           quint32 c) {

  auto point = dynamic_cast<const S57::Geometry::Point*>(obj->geometry());
  Q_ASSERT(point != nullptr);

  auto s1 = obj->attributeValue(m_sectr1).toDouble() * M_PI / 180.;
  auto s2 = obj->attributeValue(m_sectr2).toDouble() * M_PI / 180.;
  while (s2 < s1) s2 += 2 * M_PI;
  const int n0 = qMin(90, qMax(static_cast<int>(r * (s2 - s1)), 2));

  const QPointF p0(point->points()[0], point->points()[1]);

  S57::ElementData e;
  e.count = n0 + 2;
  e.offset = 0;
  e.mode = GL_LINE_STRIP_ADJACENCY_EXT;
  e.bbox = QRectF(p0 - QPointF(r, r), QSizeF(2 * r, 2 * r));

  S57::ElementDataVector elements;
  elements.append(e);

  GL::VertexVector vertices;
  // account adjacency
  vertices << 0 << 0;
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
  auto p = new S57::LineLocalData(vertices, elements, color, S52::LineWidthMM(lw),
                                  as_numeric(t), true, p0);

  return S57::PaintDataMap{{p->type(), p}};
}



S52::CSObstruction04::CSObstruction04(quint32 index)
  : Function("OBSTRN04", index)
  , m_watlev(FindIndex("WATLEV"))
  , m_catobs(FindIndex("CATOBS"))
  , m_valsou(FindIndex("VALSOU"))
  , m_uwtroc(FindIndex("UWTROC"))
  , m_uwtroc03(FindIndex("UWTROC03"))
  , m_uwtroc04(FindIndex("UWTROC04"))
  , m_danger01(FindIndex("DANGER01"))
  , m_danger51(FindIndex("DANGER51"))
  , m_danger52(FindIndex("DANGER52"))
  , m_danger53(FindIndex("DANGER03")) //  DANGER53 missing in chartsymbols
  , m_lndare01(FindIndex("LNDARE01"))
  , m_obstrn01(FindIndex("OBSTRN01"))
  , m_obstrn03(FindIndex("OBSTRN03"))
  , m_obstrn11(FindIndex("OBSTRN11"))
  , m_quapos(FindIndex("QUAPOS"))
  , m_lowacc31(FindIndex("LOWACC31"))
  , m_lowacc41(FindIndex("LOWACC41"))
  , m_foular01(FindIndex("FOULAR01"))
  , m_chblk(FindIndex("CHBLK"))
  , m_depvs(FindIndex("DEPVS"))
  , m_chgrd(FindIndex("CHGRD"))
  , m_chbrn(FindIndex("CHBRN"))
  , m_cstln(FindIndex("CSTLN"))
  , m_depit(FindIndex("DEPIT"))
  , m_isodgr01(FindIndex("ISODGR01"))
{}

S57::PaintDataMap S52::CSObstruction04::execute(const QVector<QVariant>&,
                                                const S57::Object* obj) {
  S57::PaintDataMap ps;
  double depth = S52::DefaultDepth; // always dry land

  int watlev = 0;
  if (obj->attributeValue(m_watlev).isValid()) {
    watlev = obj->attributeValue(m_watlev).toInt();
  }
  int catobs = 0;
  if (obj->attributeValue(m_catobs).isValid()) {
    catobs = obj->attributeValue(m_catobs).toInt();
  }

  if (obj->attributeValue(m_valsou).isValid()) {
    depth = obj->attributeValue(m_valsou).toDouble();
  } else if (catobs == 6 || watlev == 3) {
    depth = .01;
  } else if (watlev == 5) {
    depth = 0.;
  }

  auto wrecks = dynamic_cast<S52::CSWrecks02*>(S52::FindFunction("WRECKS02"));
  auto p = wrecks->dangerData(depth, obj);
  bool danger = p.size() == 2;
  ps += p;

  const auto t = obj->geometry()->type();
  if (t == S57::Geometry::Type::Point) {
    // continuation A
    auto fqua = dynamic_cast<S52::CSQualOfPos01*>(S52::FindFunction("QUAPOS01"));
    ps += fqua->pointData(obj);
    if (danger) {
      return ps;
    }
    quint32 sym;
    bool doSnd = false;
    if (obj->attributeValue(m_valsou).isValid()) {
      if (depth <= 20.) {
        if (obj->classCode() == m_uwtroc) {
          if (watlev == 4 || watlev == 5) {
            sym = m_uwtroc04;
          } else {
            doSnd = true;
            sym = m_danger51;
          }
        } else {
          if (watlev == 1 || watlev == 2) {
            sym = m_lndare01;
          } else if (watlev == 3) {
            doSnd = true;
            sym = m_danger52;
          } else if (watlev == 4 || watlev == 5) {
            doSnd = true;
            sym = m_danger53;
          } else {
            doSnd = true;
            sym = m_danger01;
          }
        }
      } else {
        sym = m_danger52;
      }
    } else {
      if (obj->classCode() == m_uwtroc) {
        if (watlev == 2) {
          sym = m_lndare01;
        } else if (watlev == 3) {
          sym = m_uwtroc03;
        } else {
          sym = m_uwtroc04;
        }
      } else {
        if (watlev == 1 || watlev == 2) {
          sym = m_obstrn11;
        } else if (watlev == 4 || watlev == 5) {
          sym = m_obstrn03;
        } else {
          sym = m_obstrn01;
        }
      }
    }

    const QVector<QVariant> v0 {sym, 0.};
    ps += S52::FindFunction("SY")->execute(v0, obj);

    if (doSnd) {
      auto soundings = dynamic_cast<S52::CSSoundings02*>(S52::FindFunction("SOUNDG02"));
      ps += soundings->symbols(depth, 0, obj);
    }

    return ps;
  }

  if (t == S57::Geometry::Type::Line) {
    // continuation B
    const int quapos = obj->attributeValue(m_quapos).isValid() ? obj->attributeValue(m_quapos).toInt() : 0;
    if (quapos > 1 && quapos < 10) {
      if (danger) {
        const QVector<QVariant> v0 {m_lowacc41, 0.};
        ps += S52::FindFunction("SY")->execute(v0, obj);
      } else {
        const QVector<QVariant> v0 {m_lowacc31, 0.};
        ps += S52::FindFunction("SY")->execute(v0, obj);
      }
    } else if (obj->attributeValue(m_valsou).isValid() && depth > 20.) {
      const QVector<QVariant> v0 {as_numeric(S52::LineType::Dashed), 2, m_chblk};
      ps += S52::FindFunction("LS")->execute(v0, obj);
    } else {
      const QVector<QVariant> v0 {as_numeric(S52::LineType::Dotted), 2, m_chblk};
      ps += S52::FindFunction("LS")->execute(v0, obj);
    }
    if (obj->attributeValue(m_valsou).isValid() && depth <= 20.) {
      auto soundings = dynamic_cast<S52::CSSoundings02*>(S52::FindFunction("SOUNDG02"));
      ps += soundings->symbols(depth, 0, obj);
    }
    return ps;
  }

  // continuation C
  auto fqua = dynamic_cast<S52::CSQualOfPos01*>(S52::FindFunction("QUAPOS01"));
  ps += fqua->pointData(obj);
  if (danger) {
    const QVector<QVariant> v0 {m_depvs, 255};
    ps += S52::FindFunction("AC")->execute(v0, obj);
    const QVector<QVariant> v1 {m_foular01, 0.};
    ps += S52::FindFunction("AP")->execute(v1, obj);
    const QVector<QVariant> v2 {as_numeric(S52::LineType::Dotted), 2, m_chblk};
    ps += S52::FindFunction("LS")->execute(v2, obj);
    return ps;
  }

  if (obj->attributeValue(m_valsou).isValid()) {
    if (depth <= 20.) {
      const QVector<QVariant> v0 {as_numeric(S52::LineType::Dotted), 2, m_chblk};
      ps += S52::FindFunction("LS")->execute(v0, obj);
    } else {
      const QVector<QVariant> v0 {as_numeric(S52::LineType::Dashed), 2, m_chgrd};
      ps += S52::FindFunction("LS")->execute(v0, obj);
    }
    auto soundings = dynamic_cast<S52::CSSoundings02*>(S52::FindFunction("SOUNDG02"));
    ps += soundings->symbols(depth, 0, obj);

    return ps;
  }

  if (watlev == 3 && catobs == 6) {
    const QVector<QVariant> v0 {m_depvs, 255};
    ps += S52::FindFunction("AC")->execute(v0, obj);
    const QVector<QVariant> v1 {m_foular01, 0.};
    ps += S52::FindFunction("AP")->execute(v1, obj);
    const QVector<QVariant> v2 {as_numeric(S52::LineType::Dotted), 2, m_chblk};
    ps += S52::FindFunction("LS")->execute(v2, obj);
  } else if (watlev == 1 || watlev == 2) {
    const QVector<QVariant> v0 {m_chbrn, 255};
    ps += S52::FindFunction("AC")->execute(v0, obj);
    const QVector<QVariant> v2 {as_numeric(S52::LineType::Solid), 2, m_cstln};
    ps += S52::FindFunction("LS")->execute(v2, obj);
  } else if (watlev == 4) {
    const QVector<QVariant> v0 {m_depit, 255};
    ps += S52::FindFunction("AC")->execute(v0, obj);
    const QVector<QVariant> v2 {as_numeric(S52::LineType::Dashed), 2, m_cstln};
    ps += S52::FindFunction("LS")->execute(v2, obj);
  } else {
    const QVector<QVariant> v0 {m_depvs, 255};
    ps += S52::FindFunction("AC")->execute(v0, obj);
    const QVector<QVariant> v2 {as_numeric(S52::LineType::Dotted), 2, m_chblk};
    ps += S52::FindFunction("LS")->execute(v2, obj);
  }

  return ps;
}


QStringList S52::CSObstruction04::descriptions(const QVector<QVariant>& vals,
                                               const S57::Object* obj) const {

  QStringList ds;
  if (obj->attributeValue(m_catobs).isValid()) {
    ds << S52::GetAttributeValueDescription(m_catobs, obj->attributeValue(m_catobs));
  }
  if (obj->attributeValue(m_watlev).isValid()) {
    ds << S52::GetAttributeValueDescription(m_watlev, obj->attributeValue(m_watlev));
  }

  if (obj->attributeValue(m_valsou).isValid()) {
    ds << QString("Sounding (%1m)").arg(obj->attributeValue(m_valsou).toDouble());
  }

  return ds;
}

void S52::CSObstruction04::paintIcon(QPainter& painter, const QVector<QVariant>&,
                                     const S57::Object* obj) const {
  double depth = S52::DefaultDepth; // always dry land

  int watlev = 0;
  if (obj->attributeValue(m_watlev).isValid()) {
    watlev = obj->attributeValue(m_watlev).toInt();
  }
  int catobs = 0;
  if (obj->attributeValue(m_catobs).isValid()) {
    catobs = obj->attributeValue(m_catobs).toInt();
  }

  if (obj->attributeValue(m_valsou).isValid()) {
    depth = obj->attributeValue(m_valsou).toDouble();
  } else if (catobs == 6 || watlev == 3) {
    depth = .01;
  } else if (watlev == 5) {
    depth = 0.;
  }

  auto wrecks = dynamic_cast<S52::CSWrecks02*>(S52::FindFunction("WRECKS02"));
  auto p = wrecks->dangerData(depth, obj);
  bool danger = p.size() == 2;

  if (danger) {
    QVector<QVariant> vals {m_isodgr01, 0.};
    S52::FindFunction("SY")->paintIcon(painter, vals, obj);
  }

  const auto t = obj->geometry()->type();
  if (t == S57::Geometry::Type::Point) {
    // continuation A
    auto fqua = dynamic_cast<S52::CSQualOfPos01*>(S52::FindFunction("QUAPOS01"));
    fqua->paintPoint(painter, obj);
    if (danger) {
      return;
    }
    quint32 sym;
    if (obj->attributeValue(m_valsou).isValid()) {
      if (depth <= 20.) {
        if (obj->classCode() == m_uwtroc) {
          if (watlev == 4 || watlev == 5) {
            sym = m_uwtroc04;
          } else {
            sym = m_danger51;
          }
        } else {
          if (watlev == 1 || watlev == 2) {
            sym = m_lndare01;
          } else if (watlev == 3) {
            sym = m_danger52;
          } else if (watlev == 4 || watlev == 5) {
            sym = m_danger53;
          } else {
            sym = m_danger01;
          }
        }
      } else {
        sym = m_danger52;
      }
    } else {
      if (obj->classCode() == m_uwtroc) {
        if (watlev == 2) {
          sym = m_lndare01;
        } else if (watlev == 3) {
          sym = m_uwtroc03;
        } else {
          sym = m_uwtroc04;
        }
      } else {
        if (watlev == 1 || watlev == 2) {
          sym = m_obstrn11;
        } else if (watlev == 4 || watlev == 5) {
          sym = m_obstrn03;
        } else {
          sym = m_obstrn01;
        }
      }
    }

    const QVector<QVariant> v0 {sym, 0.};
    S52::FindFunction("SY")->paintIcon(painter, v0, obj);
    return;
  }

  if (t == S57::Geometry::Type::Line) {
    // continuation B
    const int quapos = obj->attributeValue(m_quapos).isValid() ? obj->attributeValue(m_quapos).toInt() : 0;
    if (quapos > 1 && quapos < 10) {
      if (danger) {
        const QVector<QVariant> v0 {m_lowacc41, 0.};
        S52::FindFunction("SY")->paintIcon(painter, v0, obj);
      } else {
        const QVector<QVariant> v0 {m_lowacc31, 0.};
        S52::FindFunction("SY")->paintIcon(painter, v0, obj);
      }
    } else if (obj->attributeValue(m_valsou).isValid() && depth > 20.) {
      const QVector<QVariant> v0 {as_numeric(S52::LineType::Dashed), 2, m_chblk};
      S52::FindFunction("LS")->paintIcon(painter, v0, obj);
    } else {
      const QVector<QVariant> v0 {as_numeric(S52::LineType::Dotted), 2, m_chblk};
      S52::FindFunction("LS")->paintIcon(painter, v0, obj);
    }
    return;
  }

  // continuation C
  auto fqua = dynamic_cast<S52::CSQualOfPos01*>(S52::FindFunction("QUAPOS01"));
  fqua->paintPoint(painter, obj);
  if (danger) {
    const QVector<QVariant> v0 {m_depvs, 255};
    S52::FindFunction("AC")->paintIcon(painter, v0, obj);
    const QVector<QVariant> v1 {m_foular01, 0.};
    S52::FindFunction("AP")->paintIcon(painter, v1, obj);
    const QVector<QVariant> v2 {as_numeric(S52::LineType::Dotted), 2, m_chblk};
    S52::FindFunction("LS")->paintIcon(painter, v2, obj);
    return;
  }

  if (obj->attributeValue(m_valsou).isValid()) {
    if (depth <= 20.) {
      const QVector<QVariant> v0 {as_numeric(S52::LineType::Dotted), 2, m_chblk};
      S52::FindFunction("LS")->paintIcon(painter, v0, obj);
    } else {
      const QVector<QVariant> v0 {as_numeric(S52::LineType::Dashed), 2, m_chgrd};
      S52::FindFunction("LS")->paintIcon(painter, v0, obj);
    }
    return;
  }

  if (watlev == 3 && catobs == 6) {
    const QVector<QVariant> v0 {m_depvs, 255};
    S52::FindFunction("AC")->paintIcon(painter, v0, obj);
    const QVector<QVariant> v1 {m_foular01, 0.};
    S52::FindFunction("AP")->paintIcon(painter, v1, obj);
    const QVector<QVariant> v2 {as_numeric(S52::LineType::Dotted), 2, m_chblk};
    S52::FindFunction("LS")->paintIcon(painter, v2, obj);
  } else if (watlev == 1 || watlev == 2) {
    const QVector<QVariant> v0 {m_chbrn, 255};
    S52::FindFunction("AC")->paintIcon(painter, v0, obj);
    const QVector<QVariant> v2 {as_numeric(S52::LineType::Solid), 2, m_cstln};
    S52::FindFunction("LS")->paintIcon(painter, v2, obj);
  } else if (watlev == 4) {
    const QVector<QVariant> v0 {m_depit, 255};
    S52::FindFunction("AC")->paintIcon(painter, v0, obj);
    const QVector<QVariant> v2 {as_numeric(S52::LineType::Dashed), 2, m_cstln};
    S52::FindFunction("LS")->paintIcon(painter, v2, obj);
  } else {
    const QVector<QVariant> v0 {m_depvs, 255};
    S52::FindFunction("AC")->paintIcon(painter, v0, obj);
    const QVector<QVariant> v2 {as_numeric(S52::LineType::Dotted), 2, m_chblk};
    S52::FindFunction("LS")->paintIcon(painter, v2, obj);
  }
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

  switch (obj->geometry()->type()) {

  case S57::Geometry::Type::Line: return lineData(obj);
  case S57::Geometry::Type::Point: return pointData(obj);
  default: /* noop */ ;
  }

  return S57::PaintDataMap();
}

QStringList S52::CSQualOfPos01::descriptions(const QVector<QVariant>& vals, const S57::Object* obj) const {
  return QStringList {GetClassDescription(obj->classCode())};
}


void S52::CSQualOfPos01::paintIcon(QPainter& painter, const QVector<QVariant>&,
                                   const S57::Object* obj) const {

  switch (obj->geometry()->type()) {

  case S57::Geometry::Type::Line: paintLine(painter, obj);
  case S57::Geometry::Type::Point: paintPoint(painter, obj);
  default: /* noop */ ;

  }
}

S57::PaintDataMap S52::CSQualOfPos01::lineData(const S57::Object *obj) const {

  QVector<QVariant> vals;

  const QVariant quapos = obj->attributeValue(m_quapos);
  const int v = quapos.toInt();

  if (quapos.isValid() && v >= 2 && v < 10) {
    vals.append(QVariant::fromValue(m_lowacc21));
    return S52::FindFunction("LC")->execute(vals, obj);
  }

  if (!quapos.isValid()) {
    vals.append(QVariant::fromValue(int(S52::LineType::Solid)));

    if (obj->classCode() == m_coalne) {
      QVariant conrad = obj->attributeValue(m_conrad);
      if (conrad.isValid()) {
        if (conrad.toInt() == 1) {
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

S57::PaintDataMap S52::CSQualOfPos01::pointData(const S57::Object *obj) const {

  const QVariant quapos = obj->attributeValue(m_quapos);
  const int v = quapos.toInt();

  if (quapos.isValid() && v >= 2 && v < 10) {
    quint32 sym;
    switch (v) {
    case 4:
      sym = m_quapos01; break; // "PA"
    case 5:
      sym = m_quapos02; break; // "PD"
    case 7:
    case 8:
      sym = m_quapos03; break; // "REP"
    default:
      sym = m_lowacc03; // "?"
    }
    QVector<QVariant> vals {sym, 0.};
    return S52::FindFunction("SY")->execute(vals, obj);
  }

  return S57::PaintDataMap();
}

void S52::CSQualOfPos01::paintLine(QPainter& painter, const S57::Object* obj) const {
  QVector<QVariant> vals;

  const QVariant quapos = obj->attributeValue(m_quapos);
  const int v = quapos.toInt();

  if (quapos.isValid() && v >= 2 && v < 10) {
    vals.append(QVariant::fromValue(m_lowacc21));
    S52::FindFunction("LC")->paintIcon(painter, vals, obj);
    return;
  }

  if (!quapos.isValid()) {
    vals.append(QVariant::fromValue(int(S52::LineType::Solid)));

    if (obj->classCode() == m_coalne) {
      QVariant conrad = obj->attributeValue(m_conrad);
      if (conrad.isValid()) {
        if (conrad.toInt() == 1) {
          vals.append(QVariant::fromValue(3));
          vals.append(QVariant::fromValue(m_chmgf));
        }
      }
    }

    if (vals.size() == 1) {
      vals.append(QVariant::fromValue(1));
      vals.append(QVariant::fromValue(m_cstln));
    }

    S52::FindFunction("LS")->paintIcon(painter, vals, obj);
  }
}

void S52::CSQualOfPos01::paintPoint(QPainter& painter, const S57::Object* obj) const {

  const QVariant quapos = obj->attributeValue(m_quapos);
  const int v = quapos.toInt();

  if (quapos.isValid() && v >= 2 && v < 10) {
    quint32 sym;
    switch (v) {
    case 4:
      sym = m_quapos01; break; // "PA"
    case 5:
      sym = m_quapos02; break; // "PD"
    case 7:
    case 8:
      sym = m_quapos03; break; // "REP"
    default:
      sym = m_lowacc03; // "?"
    }
    QVector<QVariant> vals {sym, 0.};
    S52::FindFunction("SY")->paintIcon(painter, vals, obj);
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

  if (!obj->attributeValue(m_restrn).isValid()) return S57::PaintDataMap();

  QSet<int> s = obj->attributeSetValue(m_restrn);

  quint32 sym;
  if (s.intersects(m_set3)) {
    // continuation A
    if (s.intersects(m_set1) || s.intersects(m_set2)) {
      sym = m_entres61;
    } else if (s.intersects(m_set4)) {
      sym = m_entres71;
    } else {
      sym = m_entres51;
    }
  } else if (s.intersects(m_set1)) {
    // continuation B
    if (s.intersects(m_set2)) {
      sym = m_achres61;
    } else if (s.intersects(m_set4)) {
      sym = m_achres71;
    } else {
      sym = m_achres51;
    }
  } else if (s.intersects(m_set2)) {
    // continuation C
    if (s.intersects(m_set4)) {
      sym = m_fshres71;
    } else {
      sym = m_fshres51;
    }
  } else if (s.intersects(m_set4)) {
    sym = m_infare51;
  } else {
    sym = m_rsrdef51;
  }
  QVector<QVariant> vals {sym, 0.};
  return S52::FindFunction("SY")->execute(vals, obj);
}

QStringList S52::CSRestrEntry01::descriptions(const QVector<QVariant>& vals,
                                               const S57::Object* obj) const {

  QStringList ds;
  if (obj->attributeValue(m_restrn).isValid()) {
    ds << S52::GetAttributeValueDescription(m_restrn, obj->attributeValue(m_restrn));
  }

  return ds;
}

void S52::CSRestrEntry01::paintIcon(QPainter& painter, const QVector<QVariant>&,
                                    const S57::Object* obj) const {
  if (!obj->attributeValue(m_restrn).isValid()) return;

  QSet<int> s = obj->attributeSetValue(m_restrn);

  quint32 sym;
  if (s.intersects(m_set3)) {
    // continuation A
    if (s.intersects(m_set1) || s.intersects(m_set2)) {
      sym = m_entres61;
    } else if (s.intersects(m_set4)) {
      sym = m_entres71;
    } else {
      sym = m_entres51;
    }
  } else if (s.intersects(m_set1)) {
    // continuation B
    if (s.intersects(m_set2)) {
      sym = m_achres61;
    } else if (s.intersects(m_set4)) {
      sym = m_achres71;
    } else {
      sym = m_achres51;
    }
  } else if (s.intersects(m_set2)) {
    // continuation C
    if (s.intersects(m_set4)) {
      sym = m_fshres71;
    } else {
      sym = m_fshres51;
    }
  } else if (s.intersects(m_set4)) {
    sym = m_infare51;
  } else {
    sym = m_rsrdef51;
  }
  QVector<QVariant> vals {sym, 0.};
  S52::FindFunction("SY")->paintIcon(painter, vals, obj);
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
      const QVector<QVariant> vals {m_lowacc01, 0.};
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

QStringList S52::CSShorelineQualOfPos03::descriptions(const QVector<QVariant>& vals,
                                                      const S57::Object* obj) const {

  QStringList ds;
  if (obj->attributeValue(m_catslc).isValid()) {
    ds << S52::GetAttributeValueDescription(m_catslc, obj->attributeValue(m_catslc));
  }
  if (obj->attributeValue(m_watlev).isValid()) {
    ds << S52::GetAttributeValueDescription(m_watlev, obj->attributeValue(m_watlev));
  }
  if (obj->attributeValue(m_condtn).isValid()) {
    ds << S52::GetAttributeValueDescription(m_condtn, obj->attributeValue(m_condtn));
  }

  return ds;
}


S52::CSSoundings02::CSSoundings02(quint32 index)
  : Function("SOUNDG02", index)
  , m_chblk(FindIndex("CHBLK"))
  , m_chgrd(FindIndex("CHGRD"))
  , m_tecsou(FindIndex("TECSOU"))
  , m_soundsb1(FindIndex("SOUNDSB1"))
  , m_soundgb1(FindIndex("SOUNDGB1"))
  , m_soundsc2(FindIndex("SOUNDSC2"))
  , m_soundgc2(FindIndex("SOUNDGC2"))
  , m_soundsa1(FindIndex("SOUNDSA1"))
  , m_quasou(FindIndex("QUASOU"))
  , m_quapos(FindIndex("QUAPOS"))
  , m_doubtful_set {3, 4, 5, 8, 9}
  , m_approximate_set {2, 3, 4, 5, 6, 7, 8, 9}
{}

S57::PaintDataMap S52::CSSoundings02::execute(const QVector<QVariant>&,
                                              const S57::Object* obj) {

  if (obj->geometry()->type() != S57::Geometry::Type::Point) {
    return S57::PaintDataMap();
  }

  auto pt = dynamic_cast<const S57::Geometry::Point*>(obj->geometry());
  if (pt->points().size() < 3) {
    return S57::PaintDataMap();
  }

  S57::PaintDataMap ps;
  const auto pts = pt->points();
  for (int index = 0; index  < pts.size() / 3; index++) {
    ps += symbols(pts[3 * index + 2], index, obj);
  }

  return ps;
}

S57::PaintDataMap S52::CSSoundings02::symbols(double depth, int index,
                                              const S57::Object *obj) const {

  S57::PaintDataMap ps;

  const bool shallow = depth < Conf::MarinerParams::SafetyDepth();
  if (obj->attributeValue(m_tecsou).isValid()) {
    QSet<int> tecsou;
    auto items = obj->attributeValue(m_tecsou).toList();
    for (auto i: items) tecsou.insert(i.toUInt());
    if (tecsou.contains(6)) {
      QVector<QVariant> vals {shallow ? m_soundsb1 : m_soundgb1, 0., index};
      ps += S52::FindFunction("SY")->execute(vals, obj);
    }
  }

  bool c2 = false;

  if (obj->attributeValue(m_quasou).isValid()) {
    QSet<int> quasou;
    auto items = obj->attributeValue(m_quasou).toList();
    for (auto i: items) quasou.insert(i.toUInt());
    if (quasou.intersects(m_doubtful_set)) {
      c2 = true;
    }
  } else if (obj->attributeValue(m_quapos).isValid()) {
    auto quapos = obj->attributeValue(m_quapos).toInt();
    if (m_approximate_set.contains(quapos)) {
      c2 = true;
    }
  }

  if (c2) {
    QVector<QVariant> vals {shallow ? m_soundsc2 : m_soundgc2, 0., index};
    ps += S52::FindFunction("SY")->execute(vals, obj);
  }

  if (depth < 0) {
    const QVector<QVariant> vals {m_soundsa1, 0., index};
    ps += S52::FindFunction("SY")->execute(vals, obj);
  }

  const double cdepth = Units::Manager::instance()->depth()->fromSI(std::abs(depth));
  auto txt = QString::number(static_cast<int>(std::round(cdepth)));

  bool hasFrac = false;
  if (cdepth < 31) {
    auto lead = static_cast<int>(cdepth);
    auto frac = static_cast<int>(std::round((cdepth - lead) * 10));
    if (frac == 10) {
      lead += 1;
      frac = 0;
    }
    txt = QString::number(lead);
    if (lead < 10 || frac != 0) {
      txt = txt + QChar(0x2080 + frac);
      hasFrac = true;
      // qCDebug(CS57) << "Depth" << txt;
    }
  }

  QVector<QVariant> vs;
  vs.append(QVariant::fromValue(txt));
  vs.append(QVariant::fromValue(1)); // hcenter justified
  vs.append(QVariant::fromValue(2)); // vcenter justified
  vs.append(QVariant::fromValue(2)); // std spacing
  vs.append(QVariant::fromValue(hasFrac ? QStringLiteral("15111") :
                                          QStringLiteral("15108"))); // style, weight, slant, size
  vs.append(QVariant::fromValue(0)); // x-offset
  vs.append(QVariant::fromValue(0)); // y-offset
  vs.append(QVariant::fromValue(shallow ? m_chblk : m_chgrd)); // color
  vs.append(QVariant::fromValue(12)); // non-standard soundings text group
  vs.append(QVariant::fromValue(index));
  ps += S52::FindFunction("TX")->execute(vs, obj);

  return ps;

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
  , m_set_rigids({S52::FindIndex("BCNCAR"),
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
        //        qCDebug(CS57) << "TOPMRK01: floating";
        break;
      }
      if (isRigid(it.value())) {
        if (m_rigids.contains(topshp)) {
          topmrk = m_rigids[topshp];
        } else {
          topmrk = m_tmardef1;
        }
        //        qCDebug(CS57) << "TOPMRK01: rigid";
        break;
      }
    }
  }

  QVector<QVariant> vals {topmrk, 0.};
  return S52::FindFunction("SY")->execute(vals, obj);
}

QStringList S52::CSTopmarks01::descriptions(const QVector<QVariant>&,
                                            const S57::Object* obj) const {
  return QStringList {"hidden"};
}

void S52::CSTopmarks01::paintIcon(QPainter& painter, const QVector<QVariant>&,
                                  const S57::Object* obj) const {
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
        //        qCDebug(CS57) << "TOPMRK01: floating";
        break;
      }
      if (isRigid(it.value())) {
        if (m_rigids.contains(topshp)) {
          topmrk = m_rigids[topshp];
        } else {
          topmrk = m_tmardef1;
        }
        //        qCDebug(CS57) << "TOPMRK01: rigid";
        break;
      }
    }
  }

  QVector<QVariant> vals {topmrk, 0.};
  S52::FindFunction("SY")->paintIcon(painter, vals, obj);
}


bool S52::CSTopmarks01::isFloating(const S57::Object *obj) const {
  const quint32 cl = obj->classCode();
  return m_set_floats.contains(cl);
}

bool S52::CSTopmarks01::isRigid(const S57::Object *obj) const {
  const quint32 cl = obj->classCode();
  return m_set_rigids.contains(cl);
}

S52::CSWrecks02::CSWrecks02(quint32 index)
  : Function("WRECKS02", index)
  , m_valsou(FindIndex("VALSOU"))
  , m_watlev(FindIndex("WATLEV"))
  , m_catwrk(FindIndex("CATWRK"))
  , m_danger01(FindIndex("DANGER01"))
  , m_danger02(FindIndex("DANGER02"))
  , m_wrecks01(FindIndex("WRECKS01"))
  , m_wrecks04(FindIndex("WRECKS04"))
  , m_wrecks05(FindIndex("WRECKS05"))
  , m_chblk(FindIndex("CHBLK"))
  , m_cstln(FindIndex("CSTLN"))
  , m_chbrn(FindIndex("CHBRN"))
  , m_depit(FindIndex("DEPIT"))
  , m_depvs(FindIndex("DEPVS"))
  , m_drval1(FindIndex("DRVAL1"))
  , m_drval2(FindIndex("DRVAL2"))
  , m_isodgr01(FindIndex("ISODGR01"))
  , m_expsou(FindIndex("EXPSOU"))
{}


S57::PaintDataMap S52::CSWrecks02::execute(const QVector<QVariant>&,
                                            const S57::Object* obj) {

  S57::PaintDataMap ps;
  double depth = S52::DefaultDepth; // always dry land

  int watlev = 0;
  if (obj->attributeValue(m_watlev).isValid()) {
    watlev = obj->attributeValue(m_watlev).toInt();
  }
  int catwrk = 0;
  if (obj->attributeValue(m_catwrk).isValid()) {
    catwrk = obj->attributeValue(m_catwrk).toInt();
  }

  if (obj->attributeValue(m_valsou).isValid()) {
    depth = obj->attributeValue(m_valsou).toDouble();
  } else if (watlev == 3) {
    depth = .01;
  } else if (watlev == 5) {
    depth = 0.;
  } else if (catwrk == 1) {
    depth = 20.;
  } else if (catwrk == 2) {
    depth = 0.;
  }

  auto p = dangerData(depth, obj);
  bool danger = p.size() == 2;
  ps += p;
  auto quapos = dynamic_cast<S52::CSQualOfPos01*>(S52::FindFunction("QUAPOS01"));
  ps += quapos->pointData(obj);

  if (obj->geometry()->type() == S57::Geometry::Type::Point) {
    if (danger) {
      return ps;
    }
    // continuation A
    if (obj->attributeValue(m_valsou).isValid()) {
      if (depth <= 20.) {
        auto soundings = dynamic_cast<S52::CSSoundings02*>(S52::FindFunction("SOUNDG02"));
        ps += soundings->symbols(depth, 0, obj);
        QVector<QVariant> vals {m_danger01, 0.};
        ps += S52::FindFunction("SY")->execute(vals, obj);
      } else {
        QVector<QVariant> vals {m_danger02, 0.};
        ps += S52::FindFunction("SY")->execute(vals, obj);
      }
      return ps;
    }
    if (catwrk == 1 && watlev == 3) {
      QVector<QVariant> vals {m_wrecks04, 0.};
      ps += S52::FindFunction("SY")->execute(vals, obj);
    } else if (catwrk == 4 || catwrk == 5 || watlev == 1 || watlev == 2 || watlev == 4 || watlev == 5) {
      QVector<QVariant> vals {m_wrecks01, 0.};
      ps += S52::FindFunction("SY")->execute(vals, obj);
    } else {
      QVector<QVariant> vals {m_wrecks05, 0.};
      ps += S52::FindFunction("SY")->execute(vals, obj);
    }
    return ps;
  }
  // continuation B

  // line style
  if (danger) {
    QVector<QVariant> vals {as_numeric(S52::LineType::Dotted), 2, m_chblk};
    ps += S52::FindFunction("LS")->execute(vals, obj);
  } else if (obj->attributeValue(m_valsou).isValid()) {
    if (depth <= 20.) {
      QVector<QVariant> vals {as_numeric(S52::LineType::Dotted), 2, m_chblk};
      ps += S52::FindFunction("LS")->execute(vals, obj);
    } else {
      QVector<QVariant> vals {as_numeric(S52::LineType::Dashed), 2, m_chblk};
      ps += S52::FindFunction("LS")->execute(vals, obj);
    }
  } else if (watlev == 2) {
    QVector<QVariant> vals {as_numeric(S52::LineType::Dashed), 2, m_cstln};
    ps += S52::FindFunction("LS")->execute(vals, obj);
  } else {
    QVector<QVariant> vals {as_numeric(S52::LineType::Dotted), 2, m_cstln};
    ps += S52::FindFunction("LS")->execute(vals, obj);
  }

  // sounding, area color
  if (obj->attributeValue(m_valsou).isValid()) {
    if (depth < 20.) {
      auto soundings = dynamic_cast<S52::CSSoundings02*>(S52::FindFunction("SOUNDG02"));
      ps += soundings->symbols(depth, 0, obj);
    }
  } else if (watlev == 1 || watlev == 2) {
    QVector<QVariant> vals {m_chbrn, 255};
    ps += S52::FindFunction("AC")->execute(vals, obj);
  } else if (watlev == 4) {
    QVector<QVariant> vals {m_depit, 255};
    ps += S52::FindFunction("AC")->execute(vals, obj);
  } else {
    QVector<QVariant> vals {m_depvs, 255};
    ps += S52::FindFunction("AC")->execute(vals, obj);
  }

  return ps;
}

QStringList S52::CSWrecks02::descriptions(const QVector<QVariant>& vals,
                                          const S57::Object* obj) const {

  QStringList ds;
  if (obj->attributeValue(m_catwrk).isValid()) {
    ds << S52::GetAttributeValueDescription(m_catwrk, obj->attributeValue(m_catwrk));
  }
  if (obj->attributeValue(m_watlev).isValid()) {
    ds << S52::GetAttributeValueDescription(m_watlev, obj->attributeValue(m_watlev));
  }

  if (obj->attributeValue(m_valsou).isValid()) {
    ds << QString("Sounding (%1m)").arg(obj->attributeValue(m_valsou).toDouble());
  }

  return ds;
}

void S52::CSWrecks02::paintIcon(QPainter& painter,
                                const QVector<QVariant>&, const S57::Object* obj) const {

  double depth = S52::DefaultDepth; // always dry land

  int watlev = 0;
  if (obj->attributeValue(m_watlev).isValid()) {
    watlev = obj->attributeValue(m_watlev).toInt();
  }
  int catwrk = 0;
  if (obj->attributeValue(m_catwrk).isValid()) {
    catwrk = obj->attributeValue(m_catwrk).toInt();
  }

  if (obj->attributeValue(m_valsou).isValid()) {
    depth = obj->attributeValue(m_valsou).toDouble();
  } else if (watlev == 3) {
    depth = .01;
  } else if (watlev == 5) {
    depth = 0.;
  } else if (catwrk == 1) {
    depth = 20.;
  } else if (catwrk == 2) {
    depth = 0.;
  }

  auto p = dangerData(depth, obj);
  const bool danger = p.size() == 2;

  if (danger) {
    QVector<QVariant> vals {m_isodgr01, 0.};
    S52::FindFunction("SY")->paintIcon(painter, vals, obj);
  }

  auto quapos = dynamic_cast<S52::CSQualOfPos01*>(S52::FindFunction("QUAPOS01"));
  quapos->paintPoint(painter, obj);

  if (obj->geometry()->type() == S57::Geometry::Type::Point) {
    if (danger) {
      return;
    }
    // continuation A
    if (obj->attributeValue(m_valsou).isValid()) {
      if (depth <= 20.) {
        QVector<QVariant> vals {m_danger01, 0.};
        S52::FindFunction("SY")->paintIcon(painter, vals, obj);
      } else {
        QVector<QVariant> vals {m_danger02, 0.};
        S52::FindFunction("SY")->paintIcon(painter, vals, obj);
      }
      return;
    }
    if (catwrk == 1 && watlev == 3) {
      QVector<QVariant> vals {m_wrecks04, 0.};
      S52::FindFunction("SY")->paintIcon(painter, vals, obj);
    } else if (catwrk == 4 || catwrk == 5 || watlev == 1 || watlev == 2 || watlev == 4 || watlev == 5) {
      QVector<QVariant> vals {m_wrecks01, 0.};
      S52::FindFunction("SY")->paintIcon(painter, vals, obj);
    } else {
      QVector<QVariant> vals {m_wrecks05, 0.};
      S52::FindFunction("SY")->paintIcon(painter, vals, obj);
    }
    return;
  }
  // continuation B

  // line style
  if (danger) {
    QVector<QVariant> vals {as_numeric(S52::LineType::Dotted), 2, m_chblk};
    S52::FindFunction("LS")->paintIcon(painter, vals, obj);
  } else if (obj->attributeValue(m_valsou).isValid()) {
    if (depth <= 20.) {
      QVector<QVariant> vals {as_numeric(S52::LineType::Dotted), 2, m_chblk};
      S52::FindFunction("LS")->paintIcon(painter, vals, obj);
    } else {
      QVector<QVariant> vals {as_numeric(S52::LineType::Dashed), 2, m_chblk};
      S52::FindFunction("LS")->paintIcon(painter, vals, obj);
    }
  } else if (watlev == 2) {
    QVector<QVariant> vals {as_numeric(S52::LineType::Dashed), 2, m_cstln};
    S52::FindFunction("LS")->paintIcon(painter, vals, obj);
  } else {
    QVector<QVariant> vals {as_numeric(S52::LineType::Dotted), 2, m_cstln};
    S52::FindFunction("LS")->paintIcon(painter, vals, obj);
  }

  // area color
  if (obj->attributeValue(m_valsou).isValid()) {
    // noop
  } else if (watlev == 1 || watlev == 2) {
    QVector<QVariant> vals {m_chbrn, 255};
    S52::FindFunction("AC")->paintIcon(painter, vals, obj);
  } else if (watlev == 4) {
    QVector<QVariant> vals {m_depit, 255};
    S52::FindFunction("AC")->paintIcon(painter, vals, obj);
  } else {
    QVector<QVariant> vals {m_depvs, 255};
    S52::FindFunction("AC")->paintIcon(painter, vals, obj);
  }
}



S57::PaintDataMap S52::CSWrecks02::dangerData(double depth,
                                              const S57::Object *obj) const {

  auto limit = Conf::MarinerParams::SafetyContour();

  if (depth > limit) {
    return S57::PaintDataMap();
  }

  int expsou = 0;
  if (obj->attributeValue(m_expsou).isValid()) {
    expsou = obj->attributeValue(m_expsou).toInt();
  }

  bool danger = false;

  for (const S57::Object* underling: obj->underlings()) {
    //    qCDebug(CS57) << "[Underling:Class]" << S52::GetClassInfo(underling->classCode());
    //    qCDebug(CS57) << "[Overling:Location]" << obj->geometry()->centerLL().print();
    //    qCDebug(CS57) << "[Limit]" << limit;
    //    for (auto k: underling->attributes().keys()) {
    //      qCDebug(CS57) << GetAttributeInfo(k, underling);
    //    }
    if (underling->geometry()->type() == S57::Geometry::Type::Line) {
      if (!underling->attributeValue(m_drval2).isValid()) continue;
      if (underling->attributeValue(m_drval2).toDouble() < limit) {
        danger = true;
        break;
      }
    } else {
      if (!underling->attributeValue(m_drval1).isValid()) continue;
      if (underling->attributeValue(m_drval1).toDouble() >= limit && expsou != 1) {
        danger = true;
        break;
      }
    }
  }

  if (!danger) {
    return S57::PaintDataMap();
  }

  int watlev = 0;
  if (obj->attributeValue(m_watlev).isValid()) {
    watlev = obj->attributeValue(m_watlev).toInt();
  }

  if (watlev == 1 || watlev == 2) {
    auto p = new S57::OverrideData(false);
    return S57::PaintDataMap{{p->type(), p}};
  }

  auto p = new S57::OverrideData(true);
  S57::PaintDataMap ps{{p->type(), p}};

  QVector<QVariant> vals {m_isodgr01, 0.};
  ps += S52::FindFunction("SY")->execute(vals, obj);

  return ps;
}

S52::CSSymbolInsert01::CSSymbolInsert01(quint32 index)
  : Function("SYMINS01", index)
  , m_symins(S52::FindIndex("SYMINS"))
  , m_clsnam(S52::FindIndex("CLSNAM"))
{}

S57::PaintDataMap S52::CSSymbolInsert01::execute(const QVector<QVariant>&,
                                                 const S57::Object* obj) {
  if (!obj->attributeValue(m_clsnam).isValid()) {
    return S57::PaintDataMap();
  }
  const auto name = obj->attributeValue(m_clsnam).toString();

  // qCDebug(CS52) << name << obj->geometry()->centerLL().print();

  if (!m_lookups.contains(name)) {
    if (!obj->attributeValue(m_symins).isValid()) {
      return S57::PaintDataMap();
    }
    const auto instr = obj->attributeValue(m_symins).toString();
    qCDebug(CS52) << name << "-" << instr;
    // All but instr are dummy values
    Lookup* lup = new Lookup(S52::Lookup::Type::PaperChart, 0, 0, 0, Lookup::Category::Base, Lookup::AttributeMap(), "", instr);
    ParseInstruction(lup);
    m_lookups[name] = lup;
  }
  return m_lookups[name]->execute(obj);
}

QStringList S52::CSSymbolInsert01::descriptions(const QVector<QVariant>&,
                                                const S57::Object*) const {
  qCWarning(CS52) << "Not implemented";
  return QStringList();
}


S52::CSGSHHSMapper::CSGSHHSMapper(quint32 index)
  : Function("GSHHSC01", index)
  , m_clsdef(S52::FindIndex("CLSDEF"))
  , m_depvs(S52::FindIndex("DEPVS"))
  , m_landa(S52::FindIndex("LANDA"))
  , m_chblk(S52::FindIndex("CHBLK"))
{}

S57::PaintDataMap S52::CSGSHHSMapper::execute(const QVector<QVariant>&,
                                              const S57::Object* obj) {
  if (!obj->attributeValue(m_clsdef).isValid()) {
    return S57::PaintDataMap();
  }

  bool ok;
  const auto prio = obj->attributeValue(m_clsdef).toString().toInt(&ok);
  if (!ok || (prio < 0) || (prio > 4)) {
    return S57::PaintDataMap();
  }

  S57::PaintDataMap ps;

  S57::PaintData* p = new S57::PriorityData(prio);
  ps.insert(p->type(), p);

  quint32 colorIndex;

  if (prio == 0 || prio == 2 || prio == 4) { // sea, lake, pond
    colorIndex = m_depvs;
  } else { // land, isle
    colorIndex = m_landa;
  }

  auto geom = dynamic_cast<const S57::Geometry::Area*>(obj->geometry());
  Q_ASSERT(geom != nullptr);

  if (geom->indexed()) {
    p = new S57::TriangleElemData(geom->triangleElements(), geom->vertexOffset(),
                                  S52::GetColor(colorIndex));
  } else {
    p = new S57::TriangleArrayData(geom->triangleElements(), geom->vertexOffset(),
                                   S52::GetColor(colorIndex));
  }
  ps.insert(p->type(), p);

  if (prio == 0) return ps;

  // land, lake, isle, pond: draw black/solid/lw=1 border
  if (!geom->lineElements().isEmpty()) {
    p = new S57::LineElemData(geom->lineElements(), 0, S52::GetColor(m_chblk),
                              S52::LineWidthMM(1), as_numeric(S52::LineType::Solid));
    ps.insert(p->type(), p);
  }

  return ps;
}

QStringList S52::CSGSHHSMapper::descriptions(const QVector<QVariant>&,
                                             const S57::Object*) const {
  qCWarning(CS52) << "Not implemented";
  return QStringList();
}


