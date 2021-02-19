/* -*- coding: utf-8-unix -*-
 *
 * field.h
 *
 * Created: 2021-02-11 2021 by Jukka Sirkka
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
#pragma once

#include <QString>
#include <QVector>
#include <QDataStream>
#include <QMap>
#include <QDate>
#include "s57object.h"

#define NAMER(X) {X, #X}

namespace S57 {

class Record;

class Record {
public:

  struct Type {
    enum palette: quint8 {DS = 10, DP = 20, DH = 30, DA = 40, CR = 60, ID = 70, IO = 80,
                       IS = 90, FE = 100, VI = 110, VC = 120, VE = 130, VF = 140, N_A = 255};
    static const inline QMap<palette, QString> asString {
      NAMER(DS), NAMER(DP), NAMER(DH), NAMER(DA), NAMER(CR), NAMER(ID), NAMER(IO),
      NAMER(IS), NAMER(FE), NAMER(VI), NAMER(VC), NAMER(VE), NAMER(VF), NAMER(N_A)
    };
    Type(quint8 v): value(static_cast<palette>(v)) {}
    Type(): value(N_A) {}
    QString print() const {return asString[value];}
    palette value;
  };

  struct Topology {
    enum palette: quint8 {Spaghetti = 1, ChainNode = 2, PlanarGraph = 3, Full = 4, N_A = 255};
    static const inline QMap<palette, QString> asString {
      NAMER(Spaghetti), NAMER(ChainNode), NAMER(PlanarGraph), NAMER(Full), NAMER(N_A)
    };
    Topology(quint8 v): value(static_cast<palette>(v)) {}
    Topology(): value(N_A) {}
    QString print() const {return asString[value];}
    palette value;
  };

  struct Units {
    enum palette: quint8 {LL = 1, EN = 2, UC = 3, N_A = 255};
    static const inline QMap<palette, QString> asString {
      NAMER(LL), NAMER(EN), NAMER(UC), NAMER(N_A)
    };
    Units(quint8 v): value(static_cast<palette>(v)) {}
    Units(): value(N_A) {}
    QString print() const {return asString[value];}
    palette value;
  };

  struct Orient {
    enum palette: quint8 {Forward = 1, Reverse = 2, N_A = 255};
    static const inline QMap<palette, QString> asString {
      NAMER(Forward), NAMER(Reverse), NAMER(N_A)
    };
    Orient(quint8 v): value(static_cast<palette>(v)) {}
    Orient(): value(N_A) {}
    QString print() const {return asString[value];}
    palette value;
  };

  struct Usage {
    enum palette: quint8 {Mask = 1, Show = 2, N_A = 255};
    static const inline QMap<palette, QString> asString {
      NAMER(Mask), NAMER(Show), NAMER(N_A)
    };
    Usage(quint8 v): value(static_cast<palette>(v)) {}
    Usage(): value(N_A) {}
    QString print() const {return asString[value];}
    palette value;
  };

  struct Boundary {
    enum palette: quint8 {Exterior = 1, Interior = 2, Truncated = 3, N_A = 255};
    static const inline QMap<palette, QString> asString {
      NAMER(Exterior), NAMER(Interior), NAMER(Truncated), NAMER(N_A)
    };
    Boundary(quint8 v): value(static_cast<palette>(v)) {}
    Boundary(): value(N_A) {}
    QString print() const {return asString[value];}
    palette value;
  };

  struct TopInd {
    enum palette: quint8 {Begin = 1, End = 2, LeftFace = 3, RightFace = 4, Face = 5, N_A = 255};
    static const inline QMap<palette, QString> asString {
      NAMER(Begin), NAMER(End), NAMER(LeftFace), NAMER(RightFace), NAMER(Face), NAMER(N_A)
    };
    TopInd(quint8 v): value(static_cast<palette>(v)) {}
    TopInd(): value(N_A) {}
    QString print() const {return asString[value];}
    palette value;
  };

  struct ExPurp {
    enum palette: quint8 {New = 1, Revision = 2, N_A = 255};
    static const inline QMap<palette, QString> asString {
      NAMER(New), NAMER(Revision), NAMER(N_A)
    };
    ExPurp(quint8 v): value(static_cast<palette>(v)) {}
    ExPurp(): value(N_A) {}
    QString print() const {return asString[value];}
    palette value;
  };

  struct ProdSpec {
    enum palette: quint8 {ENC = 1, Catalogue = 2, N_A = 255};
    static const inline QMap<palette, QString> asString {
      NAMER(ENC), NAMER(Catalogue), NAMER(N_A)
    };
    ProdSpec(quint8 v): value(static_cast<palette>(v)) {}
    ProdSpec(): value(N_A) {}
    QString print() const {return asString[value];}
    palette value;
  };

  struct LexLevel {
    enum palette: quint8 {ASCII = 0, ISO8859_1 = 1, UCS2 = 2, N_A = 255};
    static const inline QMap<palette, QString> asString {
      NAMER(ASCII), NAMER(ISO8859_1), NAMER(UCS2), NAMER(N_A)
    };
    LexLevel(quint8 v): value(static_cast<palette>(v)) {}
    LexLevel(): value(N_A) {}
    QString print() const {return asString[value];}
    palette value;
  };

  struct UpdateInstr {
    enum palette: quint8 {Insert = 1, Delete = 2, Modify = 3, N_A = 255};
    static const inline QMap<palette, QString> asString {
      NAMER(Insert), NAMER(Delete), NAMER(Modify), NAMER(N_A)
    };
    UpdateInstr(quint8 v): value(static_cast<palette>(v)) {}
    UpdateInstr(): value(N_A) {}
    QString print() const {return asString[value];}
    palette value;
  };

  struct Geometry {
    enum palette: quint8 {Point = 1, Line = 2, Area = 3, N_A = 255};
    static const inline QMap<palette, QString> asString {
      NAMER(Point), NAMER(Line), NAMER(Area), NAMER(N_A)
    };
    Geometry(quint8 v): value(static_cast<palette>(v)) {}
    Geometry(): value(N_A) {}
    QString print() const {return asString[value];}
    palette value;
  };

  struct Relation {
    enum palette: quint8 {Master = 1, Slave = 2, Peer = 3, N_A = 255};
    static const inline QMap<palette, QString> asString {
      NAMER(Master), NAMER(Slave), NAMER(Peer), NAMER(N_A)
    };
    Relation(quint8 v): value(static_cast<palette>(v)) {}
    Relation(): value(N_A) {}
    QString print() const {return asString[value];}
    palette value;
  };

  static Record* Create(const QString& name, const QByteArray& bytes);

  const QString& name() const {return m_name;}
  const Type& type() const {return m_type;}
  quint32 id() const {return m_id;}

  const Record* find(const QString& name) const;

  QString records() const;

  Record* next;

  virtual ~Record() {delete next;}

protected:

  static const quint8 unitTerminator = 0x1f;
  static const quint8 attributeDeleter = 0x7f;

  Record(const QString& name)
    : next(nullptr)
    , m_name(name)
    , m_type()
    , m_id(0)
  {}

  void decodeName(QDataStream& stream);

  static S57::Attribute decode_attribute(quint16 acode, const QByteArray& v);


protected:

  QString m_name;
  Type m_type;
  quint32 m_id;

};


class DSID: public Record {
public:
  DSID(const QByteArray& bytes);
  ExPurp exPurp;
  QDate updated;
  QDate issued;
  ProdSpec prodSpec;
};

class DSSI: public Record {
public:
  DSSI(const QByteArray& bytes);
  Topology topology;
  LexLevel attfLevel;
  LexLevel nttfLevel;
  quint32 numMeta;
  quint32 numCartographic;
  quint32 numGeo;
  quint32 numCollection;
  quint32 numIsolated;
  quint32 numConnected;
  quint32 numEdge;
  quint32 numFace;
};

class DSPM: public Record {
public:
  DSPM(const QByteArray& bytes);
  S57::AttributeMap attributes;
  quint32 scale;
  Units units;
  quint32 coordFactor;
  quint32 soundingFactor;
};

class VRID: public Record {
public:
  VRID(const QByteArray& bytes);
  UpdateInstr instruction;
  quint16 version;
};

class VRPC: public Record {
public:
  VRPC(const QByteArray& bytes);
  UpdateInstr instruction;
  quint16 first;
  quint16 count;
};


class ATTV: public Record {
public:
  ATTV(const QByteArray& bytes);
  S57::AttributeMap attributes;
};

class VRPT: public Record {
public:

  struct PointerField {
    PointerField() = default;
    Type type;
    quint32 id;
    Orient orient;
    Boundary boundary;
    TopInd topind;
    Usage usage;
  };

  VRPT(const QByteArray& bytes);

  QVector<PointerField> pointers;
};

class SGCC: public Record {
public:
  SGCC(const QByteArray& bytes);
  UpdateInstr instruction;
  quint16 first;
  quint16 count;

};

class SG2D: public Record {
public:
  SG2D(const QByteArray& bytes);
  QVector<QPointF> points;
};


class SG3D: public Record {
public:
  struct Sounding {
    Sounding() = default;
    Sounding(qreal x, qreal y, float z)
      : location(x, y)
      , value(z) {}

    QPointF location;
    float value;
  };
  SG3D(const QByteArray& bytes);
  QVector<Sounding> soundings;
};

class FRID: public Record {
public:
  FRID(const QByteArray& bytes);
  Geometry geom;
  quint16 code;
  quint16 version;
  UpdateInstr instruction;
};

struct LongName {
  LongName() = default;
  LongName(QDataStream& stream);

  quint16 agency;
  quint32 id;
  quint16 subid;
};

class FOID: public Record {
public:
  FOID(const QByteArray& bytes);
  LongName id;
};

class ATTF: public Record {
public:
  ATTF(const QByteArray& bytes);
  S57::AttributeMap attributes;
};

class NATF: public Record {
public:
  NATF(const QByteArray& bytes);
  S57::AttributeMap attributes;
};

class FFPC: public Record {
public:
  FFPC(const QByteArray& bytes);
  UpdateInstr instruction;
  quint16 first;
  quint16 count;

};

class FFPT: public Record {
public:

  struct PointerField {
    PointerField() = default;
    PointerField(const LongName& i, const Relation& rel)
      : id(i)
      , relation(rel) {}

    LongName id;
    Relation relation;
  };

  FFPT(const QByteArray& bytes);

  QVector<PointerField> pointers;
};

class FSPC: public Record {
public:
  FSPC(const QByteArray& bytes);
  UpdateInstr instruction;
  quint16 first;
  quint16 count;
};

class FSPT: public Record {
public:
  struct PointerField {
    PointerField() = default;
    Type type;
    quint32 id;
    Orient orient;
    Boundary boundary;
    Usage usage;
  };

  FSPT(const QByteArray& bytes);

  QVector<PointerField> pointers;

};


} // namespace S57

template<typename T> T read_integer(QDataStream& stream, int n) {

  QByteArray bytes;
  bytes.resize(n);
  stream.readRawData(bytes.data(), n);

  return static_cast<T>(bytes.toLong());

}

template<typename T> T read_value(QDataStream& stream) {

  T value;
  stream >> value;

  return value;
}

QString read_string(QDataStream& stream, int n);
QByteArray read_string_until(QDataStream& stream, quint8 term, int& len);


