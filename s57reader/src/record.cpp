/* -*- coding: utf-8-unix -*-
 *
 * field.cpp
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
#include "record.h"
#include <QDebug>
#include <QDataStream>
#include <QPointF>
#include "s52names.h"

QString read_string(QDataStream& stream, int n) {

  QByteArray bytes;
  bytes.resize(n);
  stream.readRawData(bytes.data(), n);
  return bytes;
}

QByteArray read_string_until(QDataStream& stream, quint8 term, int& len) {

  quint8 byte;
  stream >> byte;
  len = 1;
  QByteArray bytes;
  while (byte != term) {
    bytes.append(byte);
    stream >> byte;
    len++;
  }
  return bytes;
}

QByteArray read_remaining(QDataStream& stream) {

  QByteArray bytes;
  quint8 byte;
  while (!stream.atEnd()) {
    stream >> byte;
    bytes.append(byte);
  }
  return bytes;
}


#define CASE(NAME) if (name == #NAME) return new NAME(bytes)

S57::Record* S57::Record::Create(const QString &name, const QByteArray& bytes) {
  CASE(DSID);
  CASE(DSSI);
  CASE(DSPM);
  CASE(VRID);
  CASE(VRPC);
  CASE(SGCC);
  CASE(SG2D);
  CASE(SG3D);
  CASE(VRPT);
  CASE(FRID);
  CASE(FOID);
  CASE(ATTF);
  CASE(NATF);
  CASE(ATTV);
  CASE(FSPT);
  CASE(FSPC);
  CASE(FFPT);
  CASE(FFPC);
  qDebug() << name;
  Q_ASSERT(false);
  return nullptr;
}

const S57::Record* S57::Record::find(const QString &name) const {
  auto item = next;
  while (item != nullptr && item->name() != name) {
    item = item->next;
  }
  return item;
}

QString S57::Record::records() const {
  auto names = name();
  auto item = next;
  while (item != nullptr) {
    names += "->" + item->name();
    item = item->next;
  }
  return names;
}

void S57::Record::decodeName(QDataStream& stream) {
  m_type = Type(read_value<quint8>(stream));
  m_id = read_value<quint32>(stream);
  // qDebug() << m_type.print() << m_id;
}

S57::DSID::DSID(const QByteArray& bytes): Record("DSID") {
  QDataStream stream(bytes);
  stream.setByteOrder(QDataStream::LittleEndian);
  decodeName(stream);
  int len;
  // expp, intu, dsnm, edtn, updn
  exPurp = ExPurp(read_value<quint8>(stream));
  // qDebug() << exPurp.print();
  read_value<quint8>(stream);
  read_string_until(stream, unitTerminator, len);
  read_string_until(stream, unitTerminator, len);
  read_string_until(stream, unitTerminator, len);
  // uadt, isdt
  updated = QDate::fromString(read_string(stream, 8), "yyyyMMdd");
  // qDebug() << updated;
  issued = QDate::fromString(read_string(stream, 8), "yyyyMMdd");
  // qDebug() << issued;
  // edition number, prsp, psdn, pred, prof, agen, comment
  read_string(stream, 4);
  prodSpec = ProdSpec(read_value<quint8>(stream));
  // qDebug() << prodSpec.print();
  read_string_until(stream, unitTerminator, len);
  read_string_until(stream, unitTerminator, len);
  read_value<quint8>(stream);
  read_value<quint16>(stream);
  read_string_until(stream, unitTerminator, len);
}


S57::DSSI::DSSI(const QByteArray& bytes): Record("DSSI") {
  QDataStream stream(bytes);
  stream.setByteOrder(QDataStream::LittleEndian);
  topology = Topology(read_value<quint8>(stream));
  // qDebug() << topology.print();
  attfLevel = LexLevel(read_value<quint8>(stream));
  // qDebug() << attfLevel.print();
  nttfLevel = LexLevel(read_value<quint8>(stream));
  // qDebug() << nttfLevel.print();

  // numbers of meta, carto, geo, collection, isolated, connected, edge, face

  numMeta = read_value<quint32>(stream);
  numCartographic = read_value<quint32>(stream);
  numGeo = read_value<quint32>(stream);

  numCollection = read_value<quint32>(stream);
  numIsolated = read_value<quint32>(stream);
  numConnected = read_value<quint32>(stream);

  numEdge = read_value<quint32>(stream);
  numFace = read_value<quint32>(stream);
}


S57::DSPM::DSPM(const QByteArray& bytes): Record("DSPM") {
  QDataStream stream(bytes);
  stream.setByteOrder(QDataStream::LittleEndian);
  decodeName(stream);
  // datums
  attributes[S52::FindCIndex("HORDAT")] = S57::Attribute(read_value<quint8>(stream));
  attributes[S52::FindCIndex("VERDAT")] = S57::Attribute(read_value<quint8>(stream));
  // sounding datum = vertical datum
  stream.skipRawData(1);
  scale = read_value<quint32>(stream);
  attributes[S52::FindCIndex("DUNITS")] = S57::Attribute(read_value<quint8>(stream));
  attributes[S52::FindCIndex("HUNITS")] = S57::Attribute(read_value<quint8>(stream));
  attributes[S52::FindCIndex("PUNITS")] = S57::Attribute(read_value<quint8>(stream));
  units = Units(read_value<quint8>(stream));
  // qDebug() << units.print();
  coordFactor = read_value<quint32>(stream);
  soundingFactor = read_value<quint32>(stream);

  int len;
  const QString comment = read_string_until(stream, unitTerminator, len);
  if (!comment.isEmpty()) qDebug() << "DSPM: comment" << comment;

  //  for (S57::AttributeMap::const_iterator it = attributes.cbegin();
  //       it != attributes.cend(); ++it) {
  //    qDebug() << S52::GetAttributeDescription(it.key()) << "=" <<
  //                S52::GetAttributeValueDescription(it.key(), it.value().value());
  //  }
}

S57::VRID::VRID(const QByteArray& bytes): Record("VRID") {
  QDataStream stream(bytes);
  stream.setByteOrder(QDataStream::LittleEndian);
  decodeName(stream);
  version = read_value<quint16>(stream);
  instruction = UpdateInstr(read_value<quint8>(stream));
  // qDebug() << m_type.print() << instruction.print();
}

S57::VRPC::VRPC(const QByteArray& bytes): Record("VRPC") {
  QDataStream stream(bytes);
  stream.setByteOrder(QDataStream::LittleEndian);
  instruction = UpdateInstr(read_value<quint8>(stream));
  first = read_value<quint16>(stream);
  count = read_value<quint16>(stream);
  // qDebug() << instruction.print() << first << count;
}


S57::SG2D::SG2D(const QByteArray& bytes): Record("SG2D") {
  QDataStream stream(bytes);
  stream.setByteOrder(QDataStream::LittleEndian);
  int numItems = bytes.length() / 8;
  while (numItems > 0) {
    const qreal y = read_value<qint32>(stream);
    const qreal x = read_value<qint32>(stream);
    points.append(QPointF(x, y));
    numItems--;
  }
  // qDebug() << "SG2D" << points.size();
}

S57::SG3D::SG3D(const QByteArray& bytes): Record("SG3D") {
  QDataStream stream(bytes);
  stream.setByteOrder(QDataStream::LittleEndian);
  int numItems = bytes.length() / 12;
  while (numItems > 0) {
    const qreal y = read_value<qint32>(stream);
    const qreal x = read_value<qint32>(stream);
    float depth = read_value<qint32>(stream);
    soundings.append(Sounding(x, y, depth));
    numItems--;
  }
  // qDebug() << "SG3D" << soundings.size();
}

S57::VRPT::VRPT(const QByteArray& bytes): Record("VRPT") {
  QDataStream stream(bytes);
  stream.setByteOrder(QDataStream::LittleEndian);
  int numItems = bytes.length() / 9;
  while (numItems > 0) {
    PointerField p;
    p.type = Type(read_value<quint8>(stream));
    p.id = read_value<quint32>(stream);
    p.orient = Orient(read_value<quint8>(stream));
    p.boundary = Boundary(read_value<quint8>(stream));
    p.topind = TopInd(read_value<quint8>(stream));
    p.usage = Usage(read_value<quint8>(stream));

    //    qDebug() << p.type.print() << p.id << p.orient.print()
    //             << p.boundary.print() << p.topind.print() << p.usage.print();

    pointers.append(p);

    numItems--;
  }
}

S57::FSPT::FSPT(const QByteArray& bytes): Record("FSPT") {
  QDataStream stream(bytes);
  stream.setByteOrder(QDataStream::LittleEndian);
  int numItems = bytes.length() / 8;
  while (numItems > 0) {
    PointerField p;
    p.type = Type(read_value<quint8>(stream));
    p.id = read_value<quint32>(stream);
    p.orient = Orient(read_value<quint8>(stream));
    p.boundary = Boundary(read_value<quint8>(stream));
    p.usage = Usage(read_value<quint8>(stream));

//    qDebug() << p.type.print() << p.id << p.orient.print()
//             << p.boundary.print() << p.usage.print();

    pointers.append(p);

    numItems--;
  }
}

S57::FSPC::FSPC(const QByteArray& bytes): Record("FSPC") {
  QDataStream stream(bytes);
  stream.setByteOrder(QDataStream::LittleEndian);
  instruction = UpdateInstr(read_value<quint8>(stream));
  first = read_value<quint16>(stream);
  count = read_value<quint16>(stream);
  // qDebug() << instruction.print() << first << count;
}

S57::SGCC::SGCC(const QByteArray& bytes): Record("SGCC") {
  QDataStream stream(bytes);
  stream.setByteOrder(QDataStream::LittleEndian);
  instruction = UpdateInstr(read_value<quint8>(stream));
  first = read_value<quint16>(stream);
  count = read_value<quint16>(stream);
  // qDebug() << instruction.print() << first << count;
}


S57::FRID::FRID(const QByteArray& bytes): Record("FRID") {
  QDataStream stream(bytes);
  stream.setByteOrder(QDataStream::LittleEndian);
  decodeName(stream);
  geom = Geometry(read_value<quint8>(stream));
  // skip group
  stream.skipRawData(1);
  code = read_value<quint16>(stream);
  version = read_value<quint16>(stream);
  instruction = UpdateInstr(read_value<quint8>(stream));

  // qDebug() << S52::GetClassInfo(code) << geom.print() << instruction.print();
}

S57::LongName::LongName(QDataStream &stream) {
  agency = read_value<quint16>(stream);
  id = read_value<quint32>(stream);
  subid = read_value<quint16>(stream);
}

S57::FOID::FOID(const QByteArray& bytes): Record("FOID") {
  QDataStream stream(bytes);
  stream.setByteOrder(QDataStream::LittleEndian);
  id = LongName(stream);
  // qDebug() << id.agency << id.id << id.subid;
}


S57::Attribute S57::Record::decode_attribute(quint16 acode, const QByteArray& v) {

  if (v.isEmpty()) {
    return S57::Attribute(S57::AttributeType::Any);
  }

  if (v == "?") {
    return S57::Attribute(S57::AttributeType::None);
  }

  if (v.back() == S57::Record::attributeDeleter) {
    return S57::Attribute(S57::AttributeType::Deleted);
  }

  auto t = S52::GetAttributeType(acode);

  if (t == S57::AttributeType::String) {
    return S57::Attribute(QString(v));
  }

  if (t == S57::AttributeType::Integer) {
    return S57::Attribute(v.toInt());
  }

  if (t == S57::Attribute::Type::IntegerList) {
    const QStringList tokens = QString(v).split(",");
    QVariantList vs;
    for (const QString& tok: tokens) {
      vs << QVariant::fromValue(tok.toInt());
    }
    return S57::Attribute(vs);
  }

  if (t == S57::AttributeType::Real) {
    return S57::Attribute(v.toDouble());
  }

  return S57::Attribute(S57::AttributeType::None);
}

S57::ATTV::ATTV(const QByteArray& bytes): Record("ATTV") {
  QDataStream stream(bytes);
  stream.setByteOrder(QDataStream::LittleEndian);
  int len;
  do {
    auto acode = read_value<quint16>(stream);
    auto s = read_string_until(stream, unitTerminator, len);
    attributes[acode] = decode_attribute(acode, s);
  } while (!stream.atEnd());

//  for (S57::AttributeMap::const_iterator it = attributes.cbegin();
//       it != attributes.cend(); ++it) {
//    qDebug() << S52::GetAttributeDescription(it.key()) << "=" <<
//                S52::GetAttributeValueDescription(it.key(), it.value().value());
//  }
}

S57::ATTF::ATTF(const QByteArray& bytes): Record("ATTF") {
  QDataStream stream(bytes);
  stream.setByteOrder(QDataStream::LittleEndian);
  int len;
  do {
    auto acode = read_value<quint16>(stream);
    auto s = read_string_until(stream, unitTerminator, len);
    attributes[acode] = decode_attribute(acode, s);
  } while (!stream.atEnd());

  //  for (S57::AttributeMap::const_iterator it = attributes.cbegin();
  //       it != attributes.cend(); ++it) {
  //    qDebug() << S52::GetAttributeDescription(it.key()) << "=" <<
  //                S52::GetAttributeValueDescription(it.key(), it.value().value());
  //  }
}


S57::NATF::NATF(const QByteArray& bytes): Record("NATF") {
  QDataStream stream(bytes);
  stream.setByteOrder(QDataStream::LittleEndian);
  int len;
  do {
    auto acode = read_value<quint16>(stream);
    auto s = read_string_until(stream, unitTerminator, len);
    attributes[acode] = decode_attribute(acode, s);
  } while (!stream.atEnd());

  //  for (S57::AttributeMap::const_iterator it = attributes.cbegin();
  //       it != attributes.cend(); ++it) {
  //    qDebug() << S52::GetAttributeDescription(it.key()) << "=" <<
  //                S52::GetAttributeValueDescription(it.key(), it.value().value());
  //  }
}

S57::FFPT::FFPT(const QByteArray& bytes): Record("FFPT") {
  QDataStream stream(bytes);
  stream.setByteOrder(QDataStream::LittleEndian);
  int len;
  do {
    auto id = LongName(stream);
    auto rel = Relation(read_value<quint8>(stream));

    const QString comment = read_string_until(stream, unitTerminator, len);
    if (!comment.isEmpty()) qDebug() << "FFPT: comment:" << comment;

    pointers.append(PointerField(id, rel));

  } while (!stream.atEnd());

//  for (const PointerField& pf: pointers) {
//    qDebug() << pf.id.agency << pf.id.id << pf.id.subid << pf.relation.print();
//  }
}

S57::FFPC::FFPC(const QByteArray& bytes): Record("FFPC") {
  QDataStream stream(bytes);
  stream.setByteOrder(QDataStream::LittleEndian);
  instruction = UpdateInstr(read_value<quint8>(stream));
  first = read_value<quint16>(stream);
  count = read_value<quint16>(stream);
  // qDebug() << instruction.print() << first << count;
}



