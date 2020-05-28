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
  CSDepthArea01(quint32 index)
    : Function("DEPARE01", index) {}

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
  }

#undef FUN

};

} // namespace S52
