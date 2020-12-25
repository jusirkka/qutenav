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

Lookup* FindLookup(const S57::Object* obj);
Function* FindFunction(quint32 index);
Function* FindFunction(const QString& name);
QColor GetColor(quint32 index);
QColor GetColor(const QString& name);
QVariant GetAttribute(const QString& name, const S57::Object* obj);
S57::Attribute::Type GetAttributeType(quint32 index);
QString GetAttributeName(quint32 index);
quint32 FindIndex(const QString& name);
quint32 FindIndex(const QString& name, bool* ok);
void InitPresentation();
QString FindPath(const QString& filename);
QString GetRasterFileName();
QString GetSymbolInfo(quint32 index, S52::SymbolType t);
QString GetAttributeInfo(quint32 index, const S57::Object* obj);
QString GetClassInfo(quint32 code);
bool IsMetaClass(quint32 code);

} // namespace S52
