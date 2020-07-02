#pragma once

#include <QVector>
#include <QVariant>

#include "s57object.h"

namespace S52 {


class Function {

public:

  quint32 index() const {return m_index;}
  const QString& name() {return m_name;}

  virtual S57::PaintDataMap execute(const QVector<QVariant>& vals, const S57::Object* obj) = 0;

protected:

  Function(const QString& name, quint32 index)
    : m_name(name)
    , m_index(index) {}


  ~Function() = default;

private:

  QString m_name;
  quint32  m_index;

};

class AreaColor: public Function {
public:
  AreaColor(quint32 index)
    : Function("AC", index) {}

  S57::PaintDataMap execute(const QVector<QVariant>& vals, const S57::Object* obj) override;

};

class AreaPattern: public Function {
public:
  AreaPattern(quint32 index)
    : Function("AP", index) {}

  S57::PaintDataMap execute(const QVector<QVariant>& vals, const S57::Object* obj) override;

};

class LineSimple: public Function {
public:
  LineSimple(quint32 index)
    : Function("LS", index) {}

  S57::PaintDataMap execute(const QVector<QVariant>& vals, const S57::Object* obj) override;

};

class LineComplex: public Function {
public:
  LineComplex(quint32 index)
    : Function("LC", index) {}

  S57::PaintDataMap execute(const QVector<QVariant>& vals, const S57::Object* obj) override;

};

class PointSymbol: public Function {
public:
  PointSymbol(quint32 index)
    : Function("SY", index) {}

  S57::PaintDataMap execute(const QVector<QVariant>& vals, const S57::Object* obj) override;

};

class Text: public Function {
public:
  Text(quint32 index)
    : Function("TX", index) {}

  S57::PaintDataMap execute(const QVector<QVariant>& vals, const S57::Object* obj) override;

};

class TextExtended: public Function {
public:
  TextExtended(quint32 index)
    : Function("TE", index) {}

  S57::PaintDataMap execute(const QVector<QVariant>& vals, const S57::Object* obj) override;

};


class CSDepthArea01: public Function {
public:
  CSDepthArea01(quint32 index);
  S57::PaintDataMap execute(const QVector<QVariant>&, const S57::Object* obj) override;
private:
  const quint32 m_drval1;
  const quint32 m_drval2;
  const quint32 m_depdw;
  const quint32 m_depmd;
  const quint32 m_depms;
  const quint32 m_depvs;
  const quint32 m_depit;
  const quint32 m_drgare;
  const quint32 m_drgare01;
  const quint32 m_chgrf;
};

class CSResArea02: public Function {
public:
  CSResArea02(quint32 index)
    : Function("RESARE02", index) {}

  S57::PaintDataMap execute(const QVector<QVariant>&, const S57::Object* obj) override;
};

class CSDataCov01: public Function {
public:
  CSDataCov01(quint32 index);
  S57::PaintDataMap execute(const QVector<QVariant>&, const S57::Object* obj) override;
private:
  const quint32 m_hodata01;
};

class CSDepthArea02: public Function {
public:
  CSDepthArea02(quint32 index)
    : Function("DEPARE02", index) {}

  S57::PaintDataMap execute(const QVector<QVariant>&, const S57::Object* obj) override;
};

class CSDepthContours02: public Function {
public:
  CSDepthContours02(quint32 index)
    : Function("DEPCNT02", index) {}

  S57::PaintDataMap execute(const QVector<QVariant>&, const S57::Object* obj) override;
};

class CSLights05: public Function {
public:
  CSLights05(quint32 index)
    : Function("LIGHTS05", index) {}

  S57::PaintDataMap execute(const QVector<QVariant>&, const S57::Object* obj) override;
};

class CSObstruction04: public Function {
public:
  CSObstruction04(quint32 index)
    : Function("OBSTRN04", index) {}

  S57::PaintDataMap execute(const QVector<QVariant>&, const S57::Object* obj) override;
};

class CSQualOfPos01: public Function {
public:
  CSQualOfPos01(quint32 index);

  S57::PaintDataMap execute(const QVector<QVariant>&, const S57::Object* obj) override;

private:

  const quint32 m_quapos;
  const quint32 m_lowacc21;
  const quint32 m_coalne;
  const quint32 m_conrad;
  const quint32 m_cstln;
  const quint32 m_chmgf;
  const quint32 m_quapos01;
  const quint32 m_quapos02;
  const quint32 m_quapos03;
  const quint32 m_lowacc03;
};

class CSRestrEntry01: public Function {
public:
  CSRestrEntry01(quint32 index)
    : Function("RESTRN01", index) {}

  S57::PaintDataMap execute(const QVector<QVariant>&, const S57::Object* obj) override;
};

class CSShorelineQualOfPos03: public Function {
public:
  CSShorelineQualOfPos03(quint32 index);
  S57::PaintDataMap execute(const QVector<QVariant>&, const S57::Object* obj) override;
private:
  const quint32 m_quapos;
  const quint32 m_lowacc01;
  const quint32 m_crossx01;
  const quint32 m_condtn;
  const quint32 m_cstln;
  const quint32 m_catslc;
  const quint32 m_watlev;
};

class CSEntrySoundings02: public Function {
public:
  CSEntrySoundings02(quint32 index)
    : Function("SOUNDG02", index) {}

  S57::PaintDataMap execute(const QVector<QVariant>&, const S57::Object* obj) override;
};

class CSTopmarks01: public Function {
public:
  CSTopmarks01(quint32 index)
    : Function("TOPMAR01", index) {}

  S57::PaintDataMap execute(const QVector<QVariant>&, const S57::Object* obj) override;
};

class CSWrecks02: public Function {
public:
  CSWrecks02(quint32 index)
    : Function("WRECKS02", index) {}

  S57::PaintDataMap execute(const QVector<QVariant>&, const S57::Object* obj) override;
};

class Functions {

public:

  QVector<Function*> contents;
  QMap<QString, quint32> names;

#define FUN(fun) do {\
  auto f = new fun(contents.size());\
  names[f->name()] = f->index();\
  contents.append(f);\
} while (false)

  Functions() {
    FUN(AreaColor);
    FUN(AreaPattern);
    FUN(LineSimple);
    FUN(LineComplex);
    FUN(PointSymbol);
    FUN(Text);
    FUN(TextExtended);
    FUN(CSDepthArea01);
    FUN(CSResArea02);
    FUN(CSDataCov01);
    FUN(CSDepthArea02);
    FUN(CSDepthContours02);
    FUN(CSLights05);
    FUN(CSObstruction04);
    FUN(CSQualOfPos01);
    FUN(CSRestrEntry01);
    FUN(CSShorelineQualOfPos03);
    FUN(CSEntrySoundings02);
    FUN(CSTopmarks01);
    FUN(CSWrecks02);
  }

#undef FUN

};

} // namespace S52
