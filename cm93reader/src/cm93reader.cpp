#include "cm93reader.h"
#include <QFile>
#include <QDataStream>
#include <functional>
#include <QDate>
#include "s52names.h"
#include "cm93presentation.h"
#include <QScopedPointer>
#include <QRegularExpression>
#include <QFileInfo>

const GeoProjection* CM93Reader::geoprojection() const {
  return m_proj;
}

CM93Reader::CM93Reader(const QString &name)
  : ChartFileReader(name)
  , m_m_sor(CM93::FindIndex("_m_sor"))
  , m_wgsox(CM93::FindIndex("_wgsox"))
  , m_wgsoy(CM93::FindIndex("_wgsoy"))
  , m_recdat(CM93::FindIndex("RECDAT"))
  , m_subst{{"ITDARE", S52::FindCIndex("DEPARE")},
            {"SPOGRD", S52::FindCIndex("DMPGRD")},
            {"FSHHAV", S52::FindCIndex("FSHFAC")},
            {"OFSPRD", S52::FindCIndex("CTNARE")},
            {"$ANNCH", S52::FindCIndex("VALACM")},
            {"$CYEAR", S52::FindCIndex("RYRMGV")},
            {"$VARIA", S52::FindCIndex("VALMAG")},
            {"COLMAR", S52::FindCIndex("COLOUR")},
            {"BRTFAC", S52::FindCIndex("BERTHS")},
            {"DSHAER", S52::FindCIndex("LNDMRK")},
            {"FLGSTF", S52::FindCIndex("LNDMRK")},
            {"MSTCON", S52::FindCIndex("LNDMRK")},
            {"WIMCON", S52::FindCIndex("LNDMRK")},
            {"CAIRNS", S52::FindCIndex("LNDMRK")},
            {"CEMTRY", S52::FindCIndex("LNDMRK")},
            {"CHIMNY", S52::FindCIndex("LNDMRK")},
            {"FLASTK", S52::FindCIndex("LNDMRK")},
            {"RADDOM", S52::FindCIndex("LNDMRK")},
            {"WNDMIL", S52::FindCIndex("LNDMRK")},
            {"HILARE", S52::FindCIndex("SLOGRD")},
            {"DUNARE", S52::FindCIndex("SLOGRD")},
            {"DYKARE", S52::FindCIndex("SLOGRD")},
            {"DYKCRW", S52::FindCIndex("SLOGRD")},
            {"PINGOS", S52::FindCIndex("SLOGRD")},
            {"LITHOU", S52::FindCIndex("BUISGL")},
            {"NAMFIX", S52::FindCIndex("OFSPLF")},
            {"PRDINS", S52::FindCIndex("OFSPLF")},
            {"NAMFLO", S52::FindCIndex("BOYSPP")},
            {"NATARE", S52::FindCIndex("ADMARE")},
            {"RMPARE", S52::FindCIndex("SLCONS")},
            {"SLIPWY", S52::FindCIndex("SLCONS")},
            {"LNDSTS", S52::FindCIndex("SLCONS")},
            {"TNKCON", S52::FindCIndex("SILTNK")},
            {"SILBUI", S52::FindCIndex("SILTNK")},
            {"TREPNT", S52::FindCIndex("VEGATN")},
            {"VEGARE", S52::FindCIndex("VEGATN")},
            {"DIFFUS", S52::FindCIndex("OBSTRN")},
            {"LITMOI", S52::FindCIndex("LIGHTS")},
            {"LNDPLC", S52::FindCIndex("SMCFAC")},
            {"ROADPT", S52::FindCIndex("ROADWY")},
            {"RODCRS", S52::FindCIndex("ROADWY")},
            {"SLTPAN", S52::FindCIndex("LNDRGN")},
            {"TELPHC", S52::FindCIndex("CONVYR")},
            {"WIRLNE", S52::FindCIndex("DAMCON")},
            {"_m_sor", S52::FindCIndex("M_COVR")}}
  , m_subst_attrs{{"DSHAER", {S52::FindCIndex("CATLMK"), QVariantList{QVariant::fromValue(4)}}},
                  {"FLGSTF", {S52::FindCIndex("CATLMK"), QVariantList{QVariant::fromValue(5)}}},
                  {"MSTCON", {S52::FindCIndex("CATLMK"), QVariantList{QVariant::fromValue(7)}}},
                  {"WIMCON", {S52::FindCIndex("CATLMK"), QVariantList{QVariant::fromValue(13)}}},
                  {"CAIRNS", {S52::FindCIndex("CATLMK"), QVariantList{QVariant::fromValue(1)}}},
                  {"CEMTRY", {S52::FindCIndex("CATLMK"), QVariantList{QVariant::fromValue(2)}}},
                  {"CHIMNY", {S52::FindCIndex("CATLMK"), QVariantList{QVariant::fromValue(3)}}},
                  {"FLASTK", {S52::FindCIndex("CATLMK"), QVariantList{QVariant::fromValue(6)}}},
                  {"RADDOM", {S52::FindCIndex("CATLMK"), QVariantList{QVariant::fromValue(16)}}},
                  {"WNDMIL", {S52::FindCIndex("CATLMK"), QVariantList{QVariant::fromValue(18)}}},
                  {"HILARE", {S52::FindCIndex("CATSLO"), 4}},
                  {"DUNARE", {S52::FindCIndex("CATSLO"), 3}},
                  {"DYKARE", {S52::FindCIndex("CATSLO"), 2}},
                  {"DYKCRW", {S52::FindCIndex("CATSLO"), 2}},
                  {"PINGOS", {S52::FindCIndex("CATSLO"), 6}},
                  {"NAMFIX", {S52::FindCIndex("CATOFP"), QVariantList{QVariant::fromValue(10)}}},
                  {"PRDINS", {S52::FindCIndex("CATOFP"), QVariantList{QVariant::fromValue(2)}}},
                  {"RMPARE", {S52::FindCIndex("CATSLC"), 12}},
                  {"SLIPWY", {S52::FindCIndex("CATSLC"), 13}},
                  {"LNDSTS", {S52::FindCIndex("CATSLC"), 11}},
                  {"NAMFLO", {S52::FindCIndex("CATSPM"), QVariantList{QVariant::fromValue(15)}}},
                  {"LITHOU", {S52::FindCIndex("BUISHP"), 5}},
                  {"NATARE", {S52::FindCIndex("JRSDTN"), 1}},
                  {"TNKCON", {S52::FindCIndex("CATSIL"), 2}},
                  {"SILBUI", {S52::FindCIndex("CATSIL"), 1}},
                  {"TREPNT", {S52::FindCIndex("CATVEG"), QVariantList{QVariant::fromValue(13)}}},
                  {"DIFFUS", {S52::FindCIndex("CATOBS"), 3}},
                  {"LITMOI", {S52::FindCIndex("CATLIT"), QVariantList{QVariant::fromValue(16)}}},
                  {"LNDPLC", {S52::FindCIndex("CATSCF"), QVariantList{QVariant::fromValue(28)}}},
                  {"RODCRS", {S52::FindCIndex("CATROD"), 7}},
                  {"SLTPAN", {S52::FindCIndex("CATLND"), QVariantList{QVariant::fromValue(15)}}},
                  {"TELPHC", {S52::FindCIndex("CATCON"), 1}},
                  {"WIRLNE", {S52::FindCIndex("CATDAM"), 1}}}
  , m_proj(GeoProjection::CreateProjection("CM93Mercator"))
{}

//    CM93 Decode support table
static quint8 Decode_Table[] = {
  0x61,0x5C,0x3F,0xAE,0x4C,0x4A,0x2F,0x1E,0x24,0x9E,0x1C,0xD6,0x80,0x0D,0x73,0x7E,
  0x37,0xEF,0x41,0xCB,0xED,0x93,0x57,0xF0,0xF6,0xAD,0xB8,0xBC,0x40,0xDD,0xCC,0x95,
  0x97,0x47,0xFC,0xAB,0xAA,0x99,0xF8,0x88,0xE4,0xB3,0x3E,0x84,0x21,0x98,0x83,0x20,
  0xBF,0x12,0x9B,0x35,0x5E,0xD7,0x04,0x50,0x3B,0xB4,0xDF,0x7F,0x23,0xEC,0xD4,0x81,
  0x03,0x31,0xEA,0x8D,0x6C,0xBA,0x1D,0x54,0x8A,0x22,0xFD,0xBD,0xDA,0xBE,0xA8,0xC6,
  0x86,0x36,0x25,0x74,0xFB,0x1B,0x8B,0xEE,0xAC,0xF5,0x08,0x7A,0xCA,0x7B,0x15,0xA9,
  0x18,0x5A,0xC8,0xD0,0x2B,0x05,0x19,0x91,0x11,0xF9,0x8E,0xE3,0x71,0x7C,0x69,0x68,
  0x72,0xCE,0x16,0x07,0x14,0xF1,0xCD,0x8C,0xF2,0xD3,0xD8,0x92,0x2C,0x49,0xA6,0xE1,
  0xE5,0xC7,0xBB,0xDB,0x67,0x7D,0x0B,0x3A,0x8F,0x59,0xC4,0xE2,0x56,0x13,0x39,0xC1,
  0x9C,0x77,0x64,0x75,0x53,0xFA,0x2E,0xDE,0xA2,0xD1,0x82,0xA7,0x48,0xA5,0x2D,0x0F,
  0x3D,0x38,0x46,0x0C,0x4B,0xE7,0x6A,0x5F,0xC2,0x1F,0xD2,0x60,0x0A,0x6D,0xB9,0x0E,
  0x5D,0x10,0x17,0x4E,0xA1,0xCF,0xFE,0x62,0xA3,0x58,0x42,0x89,0x5B,0x85,0x70,0xD5,
  0xC9,0x43,0x06,0x27,0x96,0x00,0x78,0xDC,0x34,0xAF,0x30,0x3C,0xE0,0x4F,0xD9,0x44,
  0xB7,0x52,0x87,0x79,0x02,0xA0,0xB5,0x94,0x28,0xF4,0xC0,0xF7,0x6B,0x55,0xB2,0x65,
  0xE9,0xEB,0x01,0x9F,0x6E,0x45,0x9A,0xC3,0x63,0x09,0x9D,0x32,0x90,0x33,0xFF,0xB1,
  0x2A,0x6F,0x29,0xF3,0xA4,0x51,0x26,0xB6,0xB0,0xC5,0x76,0xE8,0x4D,0x1A,0xE6,0x66
};

template<typename T> T read_and_decode(QDataStream& stream) {

  if (stream.atEnd()) throw ChartFileError("At end of chart file, cannot read more data");

  T t;
  stream >> t;

  // decode inplace
  quint8 *q = reinterpret_cast<quint8*>(&t);

  for (uint i = 0; i < sizeof(T); i++) {
    quint8 a = *q;
    *q = Decode_Table[a];

    q++;
  }

  return t;
}

QByteArray read_and_decode_bytes(QDataStream& stream, int n) {

  if (stream.atEnd()) throw ChartFileError("At end of chart file, cannot read more data");

  QByteArray bytes;
  bytes.resize(n);
  stream.readRawData(bytes.data(), n);


  for (int i = 0; i < bytes.size(); i++) {
    quint8 a = bytes[i];
    bytes[i] = Decode_Table[a];
  }

  return bytes;
}

namespace CM93 {

class Attribute {
public:

  static Attribute* Decode(QDataStream& stream);

  const QString& name() const {return m_name;}
  const QString& type() const {return m_type;}
  const QVariant& value() const {return m_value;}
  quint8 index() const {return m_index;}
  quint16 bytesDecoded() const {return m_bytes;}

  virtual ~Attribute() = default;

  virtual void decode(QDataStream& stream) = 0;
  virtual S57::Attribute attribute(S57::Attribute::Type t, bool* ok) const = 0;

  Attribute(quint8 index, const QString& name, const QString& type)
    : m_index(index), m_name(name), m_type(type), m_bytes(1)
  {}

protected:

  quint8 m_index;
  QString m_name;
  QString m_type;
  QVariant m_value;
  quint16 m_bytes;
};

class StringAttribute: public Attribute {
public:
  explicit StringAttribute(quint8 index)
    : Attribute(index, CM93::GetAttributeName(index), "String") {}

  void decode(QDataStream& stream) override {
    QByteArray bytes;
    auto b = read_and_decode<quint8>(stream);
    while (b != 0) {
      bytes.append(b);
      b = read_and_decode<quint8>(stream);
    }
    QString v = QString::fromUtf8(bytes);
    m_value = QVariant::fromValue(v);
    m_bytes += 1 + bytes.length();
  }

  S57::Attribute attribute(S57::Attribute::Type t, bool* ok) const override {
    *ok = true;
    if (t == S57::Attribute::Type::String) {
      return S57::Attribute(m_value.toString());
    }
    if (t == S57::Attribute::Type::Real) {
      // 12.00 E
      const QRegularExpression re(QString("(\\d+\\.\\d+) (E|W)"));
      QRegularExpressionMatch m = re.match(m_value.toString());
      if (m.hasMatch()) {
        double v = m.captured(1).toDouble();
        v *= (m.captured(2) == "W" ? -1. : 1.);
        return S57::Attribute(v);
      }
    }
    *ok = false;
    return S57::Attribute();
  }

};

class ByteAttribute: public Attribute {
public:
  explicit ByteAttribute(quint8 index)
    : Attribute(index, CM93::GetAttributeName(index), "Byte") {}

  void decode(QDataStream& stream) override {
    auto b = read_and_decode<quint8>(stream);
    m_value = QVariant::fromValue(b);
    m_bytes += 1;
  }

  S57::Attribute attribute(S57::Attribute::Type t, bool* ok) const override {
    *ok = true;
    if (t == S57::Attribute::Type::Integer) {
      return S57::Attribute(m_value.toInt());
    }
    if (t == S57::Attribute::Type::IntegerList && m_name == "COLMAR") {
      QVector<int> values;
      switch (m_value.toInt()) {
      // from opencpn/cm93.cpp
      case 1: values << 4; break; // green
      case 2: values << 2; break; // black
      case 3: values << 3; break; // red
      case 4: values << 6; break; // yellow
      case 5: values << 1; break; // white
      case 6: values << 11; break; // orange
      case 7: values << 2 << 6; break; // black/yellow
      case 8: values << 2 << 6 << 2; break; // black/yellow/black
      case 9: values << 6 << 2; break; // yellow/black
      case 10: values << 2 << 6; break; // yellow/black/yellow
      case 11: values << 3 << 1; break; // red/white
      case 12: values << 4 << 3 << 4; break; // green/red/green
      case 13: values << 3 << 4 << 3; break; // red/green/red
      case 14: values << 2 << 3 << 2; break; // black/red/black
      case 15: values << 6 << 3 << 6; break; // yellow/red/yellow
      case 16: values << 4 << 3; break; // green/red
      case 17: values << 3 << 4; break; // red/green
      case 18: values << 4 << 1; break; // green/white
      default: *ok = false;
      }
      QVariantList variants;
      for (auto v: values) variants << QVariant::fromValue(v);
      return S57::Attribute(variants);
    }
    if (t == S57::Attribute::Type::IntegerList && lists.contains(m_name)) {
      return S57::Attribute(QVariantList {m_value});
    }
    *ok = false;
    return S57::Attribute();
  }

private:

  static const inline QStringList lists {"COLPAT", "CATSPM", "CATROS",
                                         "CATLIT", "QUASOU", "CATPIP"};

};

class ListAttribute: public Attribute {
public:
  explicit ListAttribute(quint8 index)
    : Attribute(index, CM93::GetAttributeName(index), "List") {}

  void decode(QDataStream& stream) override {
    auto n_elems = read_and_decode<quint8>(stream);
    QVariantList elems;
    for (int i = 0; i < n_elems; i++) {
      auto v = read_and_decode<quint8>(stream);
      elems << QVariant::fromValue(static_cast<int>(v));
    }
    m_value = QVariant::fromValue(elems);
    m_bytes += n_elems + 1;
  }

  S57::Attribute attribute(S57::Attribute::Type t, bool* ok) const override {
    *ok = true;
    if (t == S57::Attribute::Type::IntegerList) {
      return S57::Attribute(m_value.toList());
    }
    *ok = false;
    return S57::Attribute();
  }

};

class WordAttribute: public Attribute {
public:
  explicit WordAttribute(quint8 index)
    : Attribute(index, CM93::GetAttributeName(index), "Word10") {}

  void decode(QDataStream& stream) override {
    auto v16 = read_and_decode<quint16>(stream);
    auto v = .1 * static_cast<double>(v16);
    m_value = QVariant::fromValue(v);
    m_bytes += 2;
  }

  S57::Attribute attribute(S57::Attribute::Type t, bool* ok) const override {
    *ok = true;
    if (t == S57::Attribute::Type::Real) {
      return S57::Attribute(m_value.toDouble());
    }
    *ok = false;
    return S57::Attribute();
  }

};

class FloatAttribute: public Attribute {
public:
  explicit FloatAttribute(quint8 index)
    : Attribute(index, CM93::GetAttributeName(index), "Real") {}

  void decode(QDataStream& stream) override {
    auto v = read_and_decode<float>(stream);
    m_value = QVariant::fromValue(v);
    m_bytes += 4;
  }

  S57::Attribute attribute(S57::Attribute::Type t, bool* ok) const override {
    *ok = true;
    if (t == S57::Attribute::Type::Real) {
      return S57::Attribute(m_value.toDouble());
    }
    if (t == S57::Attribute::Type::Integer) {
      auto v = m_value.toDouble();
      if (v - std::floor(v) == 0.) {
        return S57::Attribute(static_cast<int>(v));
      }
    }
    if (t == S57::Attribute::Type::String && m_name == "$CYEAR") {
      return S57::Attribute(QString::number(static_cast<int>(m_value.toDouble())));
    }
    *ok = false;
    return S57::Attribute();
  }

};

class LongAttribute: public Attribute {
public:
  explicit LongAttribute(quint8 index)
    : Attribute(index, CM93::GetAttributeName(index), "Int") {}

  void decode(QDataStream& stream) override {
    auto v = read_and_decode<int>(stream);
    m_value = QVariant::fromValue(v);
    m_bytes += 4;
  }

  S57::Attribute attribute(S57::Attribute::Type t, bool* ok) const override {
    *ok = true;
    if (t == S57::Attribute::Type::Integer) {
      return S57::Attribute(m_value.toInt());
    }
    *ok = false;
    return S57::Attribute();
  }
};

class TextAttribute: public Attribute {
public:
  explicit TextAttribute(quint8 index)
    : Attribute(index, CM93::GetAttributeName(index), "Text") {}

  void decode(QDataStream& stream) override {
    stream.skipRawData(3);

    QByteArray bytes;
    auto b = read_and_decode<quint8>(stream);
    while (b != 0) {
      bytes.append(b);
      b = read_and_decode<quint8>(stream);
    }
    QString v = QString::fromUtf8(bytes);
    m_value = QVariant::fromValue(v);
    m_bytes += 4 + bytes.length();
  }

  S57::Attribute attribute(S57::Attribute::Type t, bool* ok) const override {
    *ok = true;
    if (t == S57::Attribute::Type::String) {
      return S57::Attribute(m_value.toString());
    }
    *ok = false;
    return S57::Attribute();
  }

};


Attribute* Attribute::Decode(QDataStream &stream) {
  auto index = read_and_decode<quint8>(stream);
  auto t = CM93::GetAttributeType(index);
  Attribute* a;
  switch (t) {
  case CM93::DataType::String: a = new StringAttribute(index); break;
  case CM93::DataType::Byte: a = new ByteAttribute(index); break;
  case CM93::DataType::List: a = new ListAttribute(index); break;
  case CM93::DataType::Word: a = new WordAttribute(index); break;
  case CM93::DataType::Float: a = new FloatAttribute(index); break;
  case CM93::DataType::Long: a = new LongAttribute(index); break;
  case CM93::DataType::Text: a = new TextAttribute(index); break;
  default: throw ChartFileError(QString("Unimplemented attribute type %1/%2")
                                .arg(index).arg(as_numeric(t)));
  }
  a->decode(stream);
  return a;
}

}

using Region = S57ChartOutline::Region;

//static void tognuplot(const Region& cov, const WGS84Point& sw,
//                      const WGS84Point& ne, const GeoProjection* gp,
//                      const QString& path) {

//  if (sw.lng() < -10) return;
//  if (ne.lng() > -5) return;
//  if (sw.lat() < 35) return;
//  if (ne.lat() > 37) return;

//  const QString gpath = QString("gnuplot") + path.right(2);
//  qDebug() << "writing to" << gpath;
//  QFile file(gpath);
//  file.open(QFile::ReadWrite);
//  QTextStream stream(&file);
//  stream.seek(file.size());

//  QPointF p;
//  stream << "\n";
//  p = gp->fromWGS84(sw);
//  stream << p.x() << " " << p.y() << " 0\n";

//  p = gp->fromWGS84(WGS84Point::fromLL(ne.lng(), sw.lat()));
//  stream << p.x() << " " << p.y() << " 0\n";

//  p = gp->fromWGS84(ne);
//  stream << p.x() << " " << p.y() << " 0\n";

//  p = gp->fromWGS84(WGS84Point::fromLL(sw.lng(), ne.lat()));
//  stream << p.x() << " " << p.y() << " 0\n";

//  p = gp->fromWGS84(sw);
//  stream << p.x() << " " << p.y() << " 0\n";
//  stream << "\n";

//  for (const WGS84PointVector& ws: cov) {
//    for (const WGS84Point& w: ws) {
//      p = gp->fromWGS84(w);
//      stream << p.x() << " " << p.y() << " 1\n";
//    }
//    p = gp->fromWGS84(ws.first());
//    stream << p.x() << " " << p.y() << " 1\n\n";
//  }
//  file.close();
//}

GeoProjection* CM93Reader::configuredProjection(const QString& path) const {

  QFile file(path);
  if (!file.open(QFile::ReadOnly)) {
    throw ChartFileError(QString("Cannot open %1 for reading").arg(path));
  }
  QDataStream stream(&file);
  stream.setByteOrder(QDataStream::LittleEndian);

  // length of prolog + header (10 + 128)
  auto header_len = read_and_decode<quint16>(stream);
  if (header_len != 138) {
    throw ChartFileError(QString("%1 is not a proper CM93 file").arg(path));
  }
  auto geom_table_len = read_and_decode<qint32>(stream);
  auto feature_table_len = read_and_decode<qint32>(stream);
  if (header_len + geom_table_len + feature_table_len != file.size()) {
    throw ChartFileError(QString("%1 is not a proper CM93 file").arg(path));
  }

  // header 64/128 bytes

  stream.skipRawData(32); // lng_min, lat_min, lng_max, lat_max

  auto e_min = read_and_decode<double>(stream);
  auto n_min = read_and_decode<double>(stream);

  auto e_max = read_and_decode<double>(stream);
  auto n_max = read_and_decode<double>(stream);

  if (e_max < e_min) {
    e_max += 2 * M_PI * CM93Mercator::zC;
  }
  Q_ASSERT(n_max > n_min);

  QSizeF scaling((e_max - e_min) / 65535., (n_max - n_min) / 65535.);
  WGS84Point ref = m_proj->toWGS84(QPointF(e_min, n_min));

  auto gp = GeoProjection::CreateProjection(m_proj->className());
  gp->setReference(ref);
  gp->setScaling(scaling);
  return gp;
}

S57ChartOutline CM93Reader::readOutline(const QString& path, const GeoProjection* gp) const {

  QFile file(path);
  if (!file.open(QFile::ReadOnly)) {
    throw ChartFileError(QString("Cannot open %1 for reading").arg(path));
  }
  QDataStream stream(&file);
  stream.setByteOrder(QDataStream::LittleEndian);

  // length of prolog + header (10 + 128)
  auto header_len = read_and_decode<quint16>(stream);
  if (header_len != 138) {
    throw ChartFileError(QString("%1 is not a proper CM93 file").arg(path));
  }
  auto geom_table_len = read_and_decode<qint32>(stream);
  auto feature_table_len = read_and_decode<qint32>(stream);
  if (header_len + geom_table_len + feature_table_len != file.size()) {
    throw ChartFileError(QString("%1 is not a proper CM93 file").arg(path));
  }

  quint32 scale = 20000000;
  if (scales.contains(path.right(1))) {
    scale = scales[path.right(1)];
  }

  // header 128 bytes

  auto lng_min = read_and_decode<double>(stream);
  auto lat_min = read_and_decode<double>(stream);
  WGS84Point sw = WGS84Point::fromLL(lng_min, lat_min);
  // qDebug() << "sw" << sw.print();

  auto lng_max = read_and_decode<double>(stream);
  auto lat_max = read_and_decode<double>(stream);
  WGS84Point ne = WGS84Point::fromLL(lng_max, lat_max);
  // qDebug() << "ne" << ne.print();

  stream.skipRawData(32); // emin, nmin, emax, nmax

  // vector record table: n_vec_records * 2 + n_vec_record_points  * 4 =
  // byte size of vertex table
  auto n_vec_records = read_and_decode<quint16>(stream);
  auto n_vec_record_points = read_and_decode<quint32>(stream);

  stream.skipRawData(24);

  auto n_feat_records = read_and_decode<quint16>(stream);

  stream.skipRawData(32);

  // from this point on there are only 32 bit floats
  stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

  // vector record table
  int offset = 0;
  EdgeVector mesh;
  GL::VertexVector vertices;
  for (int i = 0; i < n_vec_records; i++) {
    auto n_elems = read_and_decode<quint16>(stream);
    for (int v = 0; v < n_elems; v++) {
      vertices << gp->scaling().width() * read_and_decode<quint16>(stream);
      vertices << gp->scaling().height() * read_and_decode<quint16>(stream);
    }
    Edge e;
    e.count = n_elems;
    e.offset = offset;
    mesh.append(e);
    offset += n_elems;
  }
  // skip rest of the geometry table
  stream.skipRawData(geom_table_len - n_vec_records * 2 - n_vec_record_points * 4);

  // feature record table
  // records with class _m_sor have coverage and publish dates
  PRegion pcov;
  QDate pub;

  bool in_sor = false;
  for (int featureId = 0; featureId < n_feat_records; featureId++) {
    auto classCode = read_and_decode<quint8>(stream);
    // qDebug() << "[class]" << CM93::GetClassInfo(classCode) << featureId << "/" << n_feat_records;
    auto objCode = read_and_decode<quint8>(stream);
    auto n_bytes = read_and_decode<quint16>(stream);
    if (classCode != m_m_sor) {
      if (in_sor) break;
      // qDebug() << "skipping" << n_bytes - 4 << "bytes";
      stream.skipRawData(n_bytes - 4);
      continue;
    }
    in_sor = true;
    auto geoType = as_enum<CM93::GeomType>(objCode & 0x0f, CM93::AllGeomTypes);
    const quint8 flags = (objCode & 0xf0) >> 4;

    auto n_records = read_and_decode<quint16>(stream);
    if (geoType == CM93::GeomType::Area ||
        geoType == CM93::GeomType::Line) {
      EdgeVector edges;
      for (int i = 0; i < n_records; i++) {
        auto edgeHeader = read_and_decode<quint16>(stream);
        auto index = edgeHeader & IndexMask;
        auto edgeflags = edgeHeader >> IndexBits;
        Q_ASSERT(index < n_vec_records);
        Edge e = mesh[index];
        e.reversed = edgeflags & ReversedBit;
        e.inner = edgeflags & InnerRingBit;
        e.border = edgeflags & BorderBit;
        edges.append(e);
      }
      pcov.append(createCoverage(vertices, edges));
    }

    if (flags & RelatedBit1) {
      auto n_elems = read_and_decode<quint8>(stream);
      stream.skipRawData(n_elems * 2);
    }

    if (flags & RelatedBit2) {
      stream.skipRawData(2);
    }

    if (flags & AttributeBit) {
      auto n_elems = read_and_decode<quint8>(stream);
      // qDebug() << "attributes" << n_elems;
      for (int i = 0; i < n_elems; i++) {
        auto a = QScopedPointer<const CM93::Attribute>(CM93::Attribute::Decode(stream));
        // qDebug() << a->name() << a->type() << a->value();
        if (a->index() == m_recdat) {
          pub = QDate::fromString(a->value().toString(), "yyyyMMdd");
          // qDebug() << "pub" << pub;
        }
      }
    }
  }

  if (!pub.isValid()) {
    throw ChartFileError(QString("Error parsing %1: Feature _m_sor not found").arg(path));
  }

  // projection without scaling
  auto gpsc = QScopedPointer<GeoProjection>(GeoProjection::CreateProjection(gp->className()));
  gpsc->setReference(gp->reference());

  const Region cov = transformCoverage(pcov, sw, ne, gpsc.data());
  // gp->setReference(WGS84Point::fromLL(-5, 35));
  // tognuplot(cov, sw, ne, gpsc.data(), path);
  return S57ChartOutline(sw, ne,
                         cov,
                         Region(),
                         gp->reference(), gp->scaling(), scale, pub, pub);
}


namespace S57 {

// Helper class to set Object's private data
class ObjectBuilder {
public:
  void cm93SetGeometry(S57::Object* obj, S57::Geometry::Base* g, const QRectF& bb) const {
    obj->m_geometry = g;
    obj->m_bbox = bb;
  }
  void cm93AddAttribute(S57::Object* obj, quint16 acode, const S57::Attribute& a) const {
    obj->m_attributes[acode] = a;
  }
};

}


void CM93Reader::readChart(GL::VertexVector& vertices,
                           GL::IndexVector& indices,
                           S57::ObjectVector& objects,
                           const QString& path,
                           const GeoProjection* proj) const {
  QFile file(path);
  if (!file.open(QFile::ReadOnly)) {
    throw ChartFileError(QString("Cannot open %1 for reading").arg(path));
  }
  QDataStream stream(&file);
  stream.setByteOrder(QDataStream::LittleEndian);

  // length of prolog + header (10 + 128)
  auto header_len = read_and_decode<quint16>(stream);
  if (header_len != 138) {
    throw ChartFileError(QString("%1 is not a proper CM93 file").arg(path));
  }

  auto geom_table_len = read_and_decode<qint32>(stream);
  auto feature_table_len = read_and_decode<qint32>(stream);
  if (header_len + geom_table_len + feature_table_len != file.size()) {
    throw ChartFileError(QString("%1 is not a proper CM93 file").arg(path));
  }

  // skip to size section
  stream.skipRawData(size_section_offset - coord_section_offset);

  // vector record table: n_vec_records * 2
  // + n_vec_record_points  * 4 = byte size of vertex table
  auto n_vec_records = read_and_decode<quint16>(stream);

  // auto n_vec_record_points = read_and_decode<quint32>(stream);
  // auto m_46 = read_and_decode<quint32>(stream);
  // qDebug() << "m_46" << m_46;
  // auto m_4a = read_and_decode<quint32>(stream);
  // qDebug() << "m_4a" << m_4a;
  stream.skipRawData(12);

  // 3d point table: n_p3d_records * 2
  // + n_p3d_record_points * 6 = byte size 3d points table
  auto n_p3d_records = read_and_decode<quint16>(stream);

  // auto n_p3d_record_points = read_and_decode<quint32>(stream);
  // auto m_54 = read_and_decode<quint32>(stream);
  // qDebug() << "m_54" << m_54;
  stream.skipRawData(8);

  // 2d point table: n_p2d_records * 4 = byte size of 2d point table
  auto n_p2d_records = read_and_decode<quint16>(stream);

  // auto m_5a = read_and_decode<quint16>(stream);
  // qDebug() << "m_5a" << m_5a;
  // auto m_5c = read_and_decode<quint16>(stream);
  // qDebug() << "m_5c" << m_5c;
  stream.skipRawData(4);

  auto n_feat_records = read_and_decode<quint16>(stream);
  // qDebug() << "Number of objects" << n_feat_records;

  // auto m_60 = read_and_decode<quint32>(stream);
  // qDebug() << "m_60" << m_60;
  // auto m_64 = read_and_decode<quint32>(stream);
  // qDebug() << "m_64" << m_64;
  // auto m_68 = read_and_decode<quint16>(stream);
  // qDebug() << "m_68" << m_68;
  // auto m_6a = read_and_decode<quint16>(stream);
  // qDebug() << "m_6a" << m_6a;
  // auto m_6c = read_and_decode<quint16>(stream);
  // qDebug() << "m_6c" << m_6c;
  // auto n_related = read_and_decode<quint32>(stream);
  // qDebug() << "num related" << n_related;
  // auto m_72 = read_and_decode<quint32>(stream);
  // qDebug() << "m_72" << m_72;
  // auto m_76 = read_and_decode<quint16>(stream);
  // qDebug() << "m_76" << m_76;
  // auto m_78 = read_and_decode<quint32>(stream);
  // qDebug() << "m_78" << m_78;
  // auto m_7c = read_and_decode<quint32>(stream);
  // qDebug() << "m_7c" << m_7c;
  stream.skipRawData(32);

  // from this point on there are only 32 bit floats
  stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

  // vector record table
  int offset = 0;
  EdgeVector mesh;
  // Apply scaling to avoid problems with non-uniform x/y scales
  for (int i = 0; i < n_vec_records; i++) {
    auto n_elems = read_and_decode<quint16>(stream);
    for (int v = 0; v < n_elems; v++) {
      vertices << proj->scaling().width() * read_and_decode<quint16>(stream);
      vertices << proj->scaling().height() * read_and_decode<quint16>(stream);
    }
    Edge e;
    e.count = n_elems;
    e.offset = offset;
    mesh.append(e);
    offset += n_elems;
  }

  // soundings table
  QVector<GL::VertexVector> soundings;
  for (int i = 0; i < n_p3d_records; i++) {
    auto n_vertices = read_and_decode<quint16>(stream);
    GL::VertexVector ss;
    for (int v = 0; v < n_vertices; v++) {
      ss << proj->scaling().width() * read_and_decode<quint16>(stream);
      ss << proj->scaling().height() * read_and_decode<quint16>(stream);
      auto z = read_and_decode<quint16>(stream);
      if (z >= 12000) {
        ss << static_cast<double>(z - 12000);
      } else {
        ss << .1 * static_cast<double>(z);
      }
    }
    soundings.append(ss);
  }

  // point table
  QVector<QPointF> points;
  for (int i = 0; i < n_p2d_records; i++) {
    auto x = proj->scaling().width() * read_and_decode<quint16>(stream);
    auto y = proj->scaling().height() * read_and_decode<quint16>(stream);
    points.append(QPointF(x, y));
  }

  // projection without scaling
  auto projSc = QScopedPointer<GeoProjection>(GeoProjection::CreateProjection(proj->className()));
  projSc->setReference(proj->reference());
  S57::ObjectBuilder helper;
  // feature record table

  for (int featureId = 0; featureId < n_feat_records; featureId++) {
    auto classCode = read_and_decode<quint8>(stream);
    auto geoHeader = read_and_decode<quint8>(stream);
    auto n_bytes = read_and_decode<quint16>(stream) - 4;
    quint32 featureCode;
    const QString className = CM93::GetClassName(classCode);
    if (m_subst.contains(className)) {
      featureCode = m_subst[className];
    } else {
      bool ok;
      featureCode = S52::FindCIndex(className, &ok);
      if (!ok) {
        qWarning() << "Unknown class" << CM93::GetClassInfo(classCode) << classCode;
        stream.skipRawData(n_bytes);
        continue;
      }
    }
    // qDebug() << CM93::GetClassInfo(classCode);
    auto object = new S57::Object(featureId, featureCode);
    objects.append(object);
    if (m_subst_attrs.contains(className)) {
      helper.cm93AddAttribute(object,
                              m_subst_attrs[className].first,
                              m_subst_attrs[className].second);
    }

    auto geoType = as_enum<CM93::GeomType>(geoHeader & 0x0f, CM93::AllGeomTypes);
    const quint8 geoFlags = geoHeader >> 4;

    switch (geoType) {
    case CM93::GeomType::Area: {
      EdgeVector edges;
      auto n_elems = read_and_decode<quint16>(stream);
      for (int i = 0; i < n_elems; i++) {
        auto edgeHeader = read_and_decode<quint16>(stream);
        auto index = edgeHeader & IndexMask;
        auto edgeflags = edgeHeader >> IndexBits;
        Q_ASSERT(index < n_vec_records);
        Edge e = mesh[index];
        e.reversed = edgeflags & ReversedBit;
        e.inner = edgeflags & InnerRingBit;
        e.border = edgeflags & BorderBit;
        edges.append(e);
      }
      S57::ElementDataVector lines;
      createLineElements(lines, indices, vertices, edges, true);

      S57::ElementDataVector triangles;
      triangulate(triangles, indices, vertices, lines);

      const QPointF center = computeAreaCenter(triangles, vertices, indices);
      const QRectF bbox = computeBBox(lines, vertices, indices);
      helper.cm93SetGeometry(object,
                             new S57::Geometry::Area(lines,
                                                     center,
                                                     triangles,
                                                     0,
                                                     true,
                                                     projSc.data()),
                             bbox);

      n_bytes -= n_elems * 2 + 2;
      break;
    }
    case CM93::GeomType::Line: {
      EdgeVector edges;
      auto n_elems = read_and_decode<quint16>(stream);
      for (int i = 0; i < n_elems; i++) {
        auto edgeHeader = read_and_decode<quint16>(stream);
        auto index = edgeHeader & IndexMask;
        auto edgeflags = edgeHeader >> IndexBits;
        Q_ASSERT(index < n_vec_records);
        Edge e = mesh[index];
        e.reversed = edgeflags & ReversedBit;
        edges.append(e);
      }
      S57::ElementDataVector lines;
      createLineElements(lines, indices, vertices, edges, false);

      const QPointF center = computeLineCenter(lines, vertices, indices);
      const QRectF bbox = computeBBox(lines, vertices, indices);
      helper.cm93SetGeometry(object,
                             new S57::Geometry::Line(lines, center, 0, projSc.data()),
                             bbox);
      n_bytes -= n_elems * 2 + 2;
      break;
    }
    case CM93::GeomType::Point: {
      auto index = read_and_decode<quint16>(stream);
      auto p0 = points[index];
      QRectF bbox(p0 - QPointF(10, 10), QSizeF(20, 20));
      helper.cm93SetGeometry(object, new S57::Geometry::Point(p0, projSc.data()), bbox);
      n_bytes -= 2;
      break;
    }
    case CM93::GeomType::Sounding: {
      auto index = read_and_decode<quint16>(stream);
      auto ps = soundings[index];
      auto bbox = computeSoundingsBBox(ps);
      helper.cm93SetGeometry(object, new S57::Geometry::Point(ps, projSc.data()), bbox);
      n_bytes -= 2;
      break;
    }
    default:
      helper.cm93SetGeometry(object, new S57::Geometry::Meta(), QRectF());
    }

    if (geoFlags & RelatedBit1) {
      auto n_elems = read_and_decode<quint8>(stream);
      stream.skipRawData(n_elems * 2);
      n_bytes -= n_elems * 2 + 1;
    }

    if (geoFlags & RelatedBit2) {
      stream.skipRawData(2);
      n_bytes -= 2;
    }

    if (geoFlags & AttributeBit) {
      auto n_elems = read_and_decode<quint8>(stream);
      n_bytes -= 1;
      for (int j = 0; j < n_elems; j++) {
        auto a = QScopedPointer<const CM93::Attribute>(CM93::Attribute::Decode(stream));
        n_bytes -= a->bytesDecoded();
        quint16 acode;
        if (m_subst.contains(a->name())) {
          acode = m_subst[a->name()];
        } else {
          bool ok;
          acode = S52::FindCIndex(a->name(), &ok);
          if (!ok) {
            qWarning() << "Unknown attribute"
                       << S52::GetClassInfo(featureCode)
                       << a->name()
                       << a->type();
            continue;
          }
        }
        bool ok;
        auto attr = a->attribute(S52::GetAttributeType(acode), &ok);
        if (!ok) {
          qWarning() << "Cannot convert attribute"
                     << S52::GetClassInfo(featureCode)
                     << a->name()
                     << a->type()
                     << a->value();
          continue;
        }
        helper.cm93AddAttribute(object, acode, attr);
      }
    }
    Q_ASSERT(n_bytes == 0);
  }
}



void CM93Reader::createLineElements(S57::ElementDataVector &elems,
                                    GL::IndexVector &indices,
                                    GL::VertexVector &vertices,
                                    const EdgeVector &edges,
                                    bool triangles) const {

  for (int i = 0; i < edges.size();) {
    S57::ElementData e;
    e.mode = GL_LINE_STRIP_ADJACENCY_EXT;
    e.offset = indices.size() * sizeof(GLuint);
    indices.append(0); // dummy index to account adjacency
    e.count = 1;
    auto start = getEndPoint(EP::First, edges[i], vertices);
    auto prevlast = start;
    while (i < edges.size() && prevlast == getEndPoint(EP::First, edges[i], vertices)) {
      e.count += addIndices(edges[i], indices);
      prevlast = getEndPoint(EP::Last, edges[i], vertices);
      i++;
    }
    // finalize current element
    const int adj = e.offset / sizeof(GLuint);
    if (prevlast == start || triangles) {
      // polygon
      if (prevlast != start) { // forcePolygons
        qDebug() << "Force polygon" << prevlast - getEndPoint(EP::First, edges[i - 1], vertices);
        // add prevlast
        indices.append(getEndPointIndex(EP::Last, edges[i - 1]));
        e.count += 1;
      }
      indices[adj] = indices.last(); // prev
      indices.append(indices[adj + 1]); // close polygon
      indices.append(indices[adj + 2]); // adjacent = next of first
    } else {
      // line string
      auto prev = indices.last();
      indices.append(getEndPointIndex(EP::Last, edges[i - 1]));
      indices.append(addAdjacent(indices.last(), prev, vertices));
      indices[adj] = addAdjacent(indices[adj + 1], indices[adj + 2], vertices);
    }
    e.count += 2;
    elems.append(e);
  }
}

using PRegion = CM93Reader::PRegion;
using PointVector = CM93Reader::PointVector;

static float area(const PointVector& ps) {
  float sum = 0;
  const int n = ps.size();
  for (int k = 0; k < n; k++) {
    const QPointF p0 = ps[k];
    const QPointF p1 = ps[(k + 1) % n];
    sum += p0.x() * p1.y() - p0.y() * p1.x();
  }
  return .5 * sum;
}

static bool checkCoverage(const PRegion& cov,
                          WGS84Point &sw,
                          WGS84Point &ne,
                          const GeoProjection *gp) {

  if (cov.isEmpty()) return false;
  //  PointVector box;
  //  box << gp->fromWGS84(sw);
  //  box << gp->fromWGS84(WGS84Point::fromLL(ne.lng(), sw.lat()));
  //  box << gp->fromWGS84(ne);
  //  box << gp->fromWGS84(WGS84Point::fromLL(sw.lng(), ne.lat()));

  // const float A = area(box);

  // compute bounding box
  QPointF ur(-1.e15, -1.e15);
  QPointF ll(1.e15, 1.e15);
  for (const PointVector& ps: cov) {
    for (const QPointF& p: ps) {
      ur.setX(qMax(ur.x(), p.x()));
      ur.setY(qMax(ur.y(), p.y()));
      ll.setX(qMin(ll.x(), p.x()));
      ll.setY(qMin(ll.y(), p.y()));
    }
  }

  const float A = (ur.x() - ll.x()) * (ur.y() - ll.y());
  float totcov = 0;
  for (const PointVector& ps: cov) {
    totcov += std::abs(area(ps) / A);
  }
  sw = gp->toWGS84(ll);
  ne = gp->toWGS84(ur);
  if (sw == WGS84Point::fromLL(ne.lng(), sw.lat())) {
    qDebug() << ll << ur;
    qDebug() << sw.lng() << sw.lat() << ne.lng() << ne.lat();
    throw ChartFileError("Too narrow coverage");
  }
  if (sw == WGS84Point::fromLL(sw.lng(), ne.lat())) {
    qDebug() << ll << ur;
    qDebug() << sw.lng() << sw.lat() << ne.lng() << ne.lat();
    throw ChartFileError("Too low coverage");
  }
  return totcov < .8;
  // return true;
}

Region CM93Reader::transformCoverage(PRegion pcov, WGS84Point &sw, WGS84Point &ne,
                                     const GeoProjection *gp) const {
  if (!checkCoverage(pcov, sw, ne, gp)) {
    return Region();
  }
  Region cov;
  for (const PointVector& ps: pcov) {
    WGS84PointVector ws;
    for (auto p: ps) {
      ws << gp->toWGS84(p);
    }
    cov.append(ws);
  }
  return cov;
}

PRegion CM93Reader::createCoverage(const GL::VertexVector &vertices,
                                   const EdgeVector &edges) const {
  PRegion cov;
  for (int i = 0; i < edges.size();) {
    PointVector ps;
    auto start = getEndPoint(EP::First, edges[i], vertices);
    auto prevlast = start;
    while (i < edges.size() && prevlast == getEndPoint(EP::First, edges[i], vertices)) {
      ps.append(addVertices(edges[i], vertices));
      prevlast = getEndPoint(EP::Last, edges[i], vertices);
      i++;
    }
    if (prevlast != start) {
      qDebug() << "Force polygon" << prevlast - getEndPoint(EP::First, edges[i - 1], vertices);
      // add prevlast
      ps << getEndPoint(EP::Last, edges[i - 1], vertices);
    }

    PointVector qs;
    const int N = ps.size();
    for (int k = 0; k < N; k++) {
      const QPointF v = ps[k];
      const QPointF vm = ps[(k + N - 1) % N];
      const QPointF vp = ps[(k + 1) % N];
      if (v.x() == vm.x() && v.x() == vp.x()) continue;
      if (v.y() == vm.y() && v.y() == vp.y()) continue;
      qs << v;
    }

    cov << qs;
  }

  return cov;
}

QPointF CM93Reader::getEndPoint(EP ep,
                                const Edge& e,
                                const GL::VertexVector& vertices) const {
  int x = 2 * e.offset;
  if ((ep == EP::Last && !e.reversed) || (ep == EP::First && e.reversed)) {
    x = 2 * (e.offset + e.count - 1);
  }
  return QPointF(vertices[x], vertices[x + 1]);
}

QPointF CM93Reader::getPoint(int index,
                             const GL::VertexVector& vertices) const {
  return QPointF(vertices[2 * index], vertices[2 * index + 1]);
}

int CM93Reader::getEndPointIndex(EP ep,
                                 const Edge& e) const {
  if ((ep == EP::Last && !e.reversed) || (ep == EP::First && e.reversed)) {
    return e.offset + e.count - 1;
  }
  return e.offset;
}

int CM93Reader::addAdjacent(int ep, int nbor, GL::VertexVector& vertices) const {
  const float x1 = vertices[2 * ep];
  const float y1 = vertices[2 * ep + 1];
  const float x2 = vertices[2 * nbor];
  const float y2 = vertices[2 * nbor + 1];
  vertices << 2 * x1 - x2 << 2 * y1 - y2;

  return (vertices.size() - 1) / 2;
}

int CM93Reader::addIndices(const Edge& e, GL::IndexVector& indices) const {
  if (!e.reversed) {
    for (int i = 0; i < e.count - 1; i++) {
      indices << e.offset + i;
    }
  } else {
    for (int i = 0; i < e.count - 1; i++) {
      indices << e.offset + e.count - i - 1;
    }
  }
  return e.count - 1;
}


CM93Reader::PointVector CM93Reader::addVertices(const Edge &e, const GL::VertexVector &vertices) const {
  PointVector ps;
  if (!e.reversed) {
    for (int i = 0; i < e.count - 1; i++) {
      ps << getPoint(e.offset + i, vertices);
    }
  } else {
    for (int i = 0; i < e.count - 1; i++) {
      ps << getPoint(e.offset + e.count - i - 1, vertices);
    }
  }
  return ps;
}



QString CM93ReaderFactory::name() const {
  return "cm93";
}

QString CM93ReaderFactory::displayName() const {
  return "CM93 Charts";
}

QStringList CM93ReaderFactory::filters() const {
  return QStringList {"*.[A-G]", "*.Z"};
}

void CM93ReaderFactory::initialize() const {
  CM93::InitPresentation();
}

ChartFileReader* CM93ReaderFactory::create() const {
  return new CM93Reader(name());
}




