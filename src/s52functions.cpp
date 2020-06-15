#include "s52functions.h"
#include "s52presentation.h"
#include "conf_marinerparams.h"
#include "settings.h"


S57::PaintDataMap S52::AreaColor::execute(const QVector<QVariant>& vals,
                                       const S57::Object* obj) {
  S57::PaintData p;

  auto geom = dynamic_cast<const S57::Geometry::Area*>(obj->geometry());
  if (!geom) return S57::PaintDataMap(); // invalid paint data

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
  if (line) {
    p.elements = line->lineElements();
    p.vertexOffset = line->vertexOffset();
  } else {
    qWarning() << "LS: not a line!?";
    return S57::PaintDataMap();
  }

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


S57::PaintDataMap S52::CSDepthArea01::execute(const QVector<QVariant>&,
                                              const S57::Object* obj) {
  auto geom = dynamic_cast<const S57::Geometry::Area*>(obj->geometry());
  if (!geom) return S57::PaintDataMap(); // invalid paint data

  // Determine the color based on mariner selections

  bool ok;
  double depth = -1.;
  const double drval1 = GetAttribute("DRVAL1", obj).toDouble(&ok);
  if (ok) {
    depth = drval1;
  }
  const double drval2 = GetAttribute("DRVAL2", obj).toDouble(&ok);
  if (ok && drval2 < depth) {
    depth = drval2;
  }


  S57::PaintData p;

  p.type = S57::PaintData::Type::Triangles;
  p.elements = geom->triangleElements();
  p.vertexOffset = geom->vertexOffset();

  const Settings* cfg = Settings::instance();

  if (cfg->twoShades() && depth >= cfg->safetyContour()) {
    p.color = S52::GetColor("DEPDW");
  } else if (depth >= cfg->deepContour()) {
    p.color = S52::GetColor("DEPDW");
  } else if (depth >= cfg->safetyContour()) {
    p.color = S52::GetColor("DEPMD");
  } else if (depth >= cfg->shallowContour()) {
    p.color = S52::GetColor("DEPMS");
  } else if (depth >= 0) {
    p.color = S52::GetColor("DEPVS");
  } else {
    p.color = S52::GetColor("DEPIT");
  }

  if (obj->classCode() != S52::FindIndex("DRGARE")) {
    return S57::PaintDataMap{{p.type, p}};
  }

  if (!GetAttribute("DRVAL1", obj).isValid()) {
    p.color = S52::GetColor("DEPMD");
  }

  S57::PaintDataMap ps{{p.type, p}};

  // run AP(DRGARE01);LS(DASH,1,CHGRF)

  QVector<QVariant> vals;
  vals.append(QVariant::fromValue(S52::FindIndex("DRGARE01")));
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
  vals.append(QVariant::fromValue(S52::FindIndex("CHGRF")));
  ps1 = S52::FindFunction("LS")->execute(vals, obj);

  for (auto it = ps1.constBegin(); it != ps1.constEnd(); ++it) {
    if (ps.contains(it.key())) {
      qWarning() << "Overwriting paint data" << quint8(it.key());
    }
    ps[it.key()] = it.value();
  }

  return ps;
}
