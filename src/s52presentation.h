#pragma once

#include <QString>
#include <QColor>
#include "s57object.h"
#include <QPoint>
#include <QRect>
#include "s52functions.h"



namespace S52 {

class ByteCoder;

class Lookup {

  friend class ByteCoder;

public:

  enum class Type: char {
    Simplified, // points
    PaperChart, // points
    Lines, // lines
    PlainBoundaries, // areas
    SymbolizedBoundaries, // areas
  };

  static const int PriorityCount = 10;

  enum class Category: char {
    Base,
    Standard,
    Other,
    Mariners
  };

  using AttributeMap = QMap<quint32, S57::Attribute>;

  Lookup(Type t, int id, quint32 code, int prioId, Category cat,
         AttributeMap attrs, QString comment, QString source)
    : m_type(t)
    , m_rcid(id)
    , m_classCode(code)
    , m_priorityId(prioId)
    , m_category(cat)
    , m_attributes(attrs)
    , m_description(comment)
    , m_source(source) {}

  Type type() const {return m_type;}
  int rcid() const {return m_rcid;}
  quint32 classCode() const {return m_classCode;}
  int priority() const {return m_priorityId;}
  Category category() const {return m_category;}
  const AttributeMap& attributes() const {return m_attributes;}
  const QString& description() const {return m_description;}
  const QString& source() const {return m_source;}
  bool byteCodeReady() const {return !m_code.isEmpty();}

  S57::PaintDataMap execute(const S57::Object* obj) const;

  // bytecode interface
  enum class Code: quint8 {Immed, Var, Fun};
  enum class Line: uint {Solid = 0x3ffff, Dashed = 0x3ffc0, Dotted = 0x30c30};

private:

  Type m_type;
  int m_rcid;
  quint32 m_classCode;
  int m_priorityId;
  Category m_category;
  AttributeMap m_attributes;
  QString m_description;
  QString m_source;

  // bytecode interface
  using CodeStack = QVector<Code>;
  using ValueStack = QVector<QVariant>;
  using ReferenceStack = QVector<quint32>;

  CodeStack m_code;
  ValueStack m_immed;
  ReferenceStack m_references;

};

class ModelData {
public:
  ModelData(int w, int h, int mind, int maxd, QPoint p, QPoint o)
    : m_box(o.x(), o.y(), w, h)
    , m_minDistance(mind), m_maxDistance(maxd)
    , m_pivot(p) {}

  ModelData() = default;

  const QRect& bbox() const {return m_box;}
  int minDistance() const {return m_minDistance;}
  int maxDistance() const {return m_maxDistance;}
  const QPoint& pivot() const {return m_pivot;}

private:

  QRect m_box;
  int m_minDistance;
  int m_maxDistance;
  QPoint m_pivot;
};

using ColorRef = QMap<char, quint32>;

class LineStyle {
public:
  LineStyle(quint32 id, const ModelData& d, const QString& c, const ColorRef& ref,
            const QString& s)
    : m_rcid(id)
    , m_modelData(d)
    , m_description(c)
    , m_colorRef(ref)
    , m_source(s)
  {}

  LineStyle() = default;

  quint32 rcid() const {return m_rcid;}
  const ModelData& modelData() const {return m_modelData;}
  const QString& description() const {return m_description;}
  const ColorRef& colorRef() const {return m_colorRef;}
  const QString& source() const {return m_source;}

  virtual ~LineStyle() = default;

private:

  quint32 m_rcid;
  ModelData m_modelData;
  QString m_description;
  ColorRef m_colorRef;
  QString m_source;
};

class Symbol: public LineStyle {
public:
  Symbol(quint32 id, const ModelData& d, const QString& c, const ColorRef& ref,
         const QString& s, bool r)
    : LineStyle(id, d, c, ref, s)
    , m_isRaster(r) {}

  Symbol() = default;

  bool isRaster() const {return m_isRaster;}

private:

  bool m_isRaster;
};

class Pattern: public Symbol {
public:
  Pattern(quint32 id, const ModelData& d, const QString& c, const ColorRef& ref,
          const QString& src, bool r, bool s, bool v)
    : Symbol(id, d, c, ref, src, r)
    , m_isStaggered(s), m_isVariableSpacing(v) {}

  Pattern() = default;

  bool isStaggered() const {return m_isStaggered;}
  bool isVariableSpacing() const {return m_isVariableSpacing;}

private:

  bool m_isStaggered;
  bool m_isVariableSpacing;
};


Lookup* FindLookup(const S57::Object* obj);
Function* FindFunction(quint32 index);
Function* FindFunction(const QString& name);
QColor GetColor(quint32 index);
QColor GetColor(const QString& name);
QVariant GetAttribute(const QString& name, const S57::Object* obj);
quint32 FindIndex(const QString& name);
void InitPresentation();

} // namespace S52
