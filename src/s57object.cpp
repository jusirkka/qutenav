#include "s57object.h"
#include <QDate>
#include <QDebug>

bool S57::Attribute::matches(const Attribute &constraint) const {
  if (constraint.type() == Type::Any) {
    if (m_type == Type::None) return false;
    return true;
  }
  if (constraint.type() == Type::None) {
    if (m_type == Type::None) return true;
    return false;
  }
  if (m_type != constraint.type()) return false;

  if (m_type == Type::Real) {
    return std::abs(m_value.toDouble() - constraint.value().toDouble()) < 1.e-6;
  }

  // Compare integer lists: Note that we require exact match
  // not just up to constraint list size
  if (m_type == Type::IntegerList) {
    QVariantList clist = constraint.value().toList();
    QVariantList mlist = m_value.toList();
    if (mlist.size() != clist.size()) return false;
    for (int i = 0; i < clist.size(); i++) {
      if (mlist[i].toInt() != clist[i].toInt()) return false;
    }
    return true;
  }

  return m_value == constraint.value();
}

void S57::Attribute::encode(QDataStream &stream) const {
  stream << static_cast<uint8_t>(m_type);
  stream << m_value;
}

S57::Attribute S57::Attribute::Decode(QDataStream &stream) {
  Attribute a;

  uint8_t t;
  stream >> t;
  a.m_type = static_cast<Attribute::Type>(t);
  stream >> a.m_value;

  return a;
}

void S57::Geometry::Base::encode(QDataStream& stream, Transform transform) const {
  stream << static_cast<uint8_t>(m_type);
  stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
  const QPointF c = transform(m_center);
  stream << c;
  stream << m_centerLL.lng() << m_centerLL.lat();
  stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
  doEncode(stream, transform);
}


S57::Geometry::Base* S57::Geometry::Base::Decode(QDataStream &stream) {
  uint8_t v;
  stream >> v;
  auto t  = static_cast<Type>(v);
  Base* geom;
  switch (t) {
  case Type::Meta:
    geom = new Meta(); break;
  case Type::Point:
    geom = new Point(); break;
  case Type::Line:
    geom = new Line(); break;
  case Type::Area:
    geom = new Area(); break;
  default:
    geom = new Meta(); break;
  }
  stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
  stream >> geom->m_center;
  double lng;
  stream >> lng;
  double lat;
  stream >> lat;
  geom->m_centerLL == WGS84Point::fromLL(lng, lat);
  stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
  geom->doDecode(stream);

  return geom;
}

void S57::Geometry::Meta::doEncode(QDataStream&/*stream*/, Transform /*transform*/) const {
  // noop
}

void S57::Geometry::Meta::doDecode(QDataStream&/*stream*/) {
  // noop
}

void S57::Geometry::Point::doEncode(QDataStream &stream, Transform transform) const {
  stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
  PointVector ps;
  if (m_points.size() == 2) {
    const QPointF p = transform(QPointF(m_points[0], m_points[1]));
    ps << p.x() << p.y();
  } else {
    const int np = m_points.size() / 3;
    for (int n = 0; n < np; n++) {
      const QPointF p = transform(QPointF(m_points[3 * n + 0], m_points[3 * n + 1]));
      ps << p.x() << p.y();
      ps << m_points[3 * n + 2];
    }
  }
  stream << ps;
  stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
}

void S57::Geometry::Point::doDecode(QDataStream &stream) {
  stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
  stream >> m_points;
  stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
}

void S57::Geometry::Line::doEncode(QDataStream& stream, Transform /*transform*/) const {
  const int ne = m_lineElements.size();
  stream << ne;
  for (const ElementData& d: m_lineElements) {
    stream << static_cast<uint>(d.mode);
    stream << static_cast<uint>(d.offset);
    stream << static_cast<uint>(d.count);
    stream << d.bbox;
  }
  stream << m_vertexOffset;
}

void S57::Geometry::Line::doDecode(QDataStream& stream) {
  int ne;
  stream >> ne;

  for (int n = 0; n < ne; n++) {
    uint v;
    ElementData d;
    stream >> v;
    d.mode = v;
    stream >> v;
    d.offset = v;
    stream >> v;
    d.count = v;
    stream >> d.bbox;
    m_lineElements.append(d);
  }

  stream >> m_vertexOffset;
}

void S57::Geometry::Area::doEncode(QDataStream& stream, Transform transform) const {
  Line::doEncode(stream, transform);
  const int ne = m_triangleElements.size();
  stream << ne;
  for (const ElementData& d: m_triangleElements) {
    stream << static_cast<uint>(d.mode);
    stream << static_cast<uint>(d.offset);
    stream << static_cast<uint>(d.count);
    stream << d.bbox;
  }
  stream << m_indexed;
}

void S57::Geometry::Area::doDecode(QDataStream& stream) {
  Line::doDecode(stream);

  int ne;
  stream >> ne;

  for (int n = 0; n < ne; n++) {
    uint v;
    ElementData d;
    stream >> v;
    d.mode = v;
    stream >> v;
    d.offset = v;
    stream >> v;
    d.count = v;
    stream >> d.bbox;
    m_triangleElements.append(d);
  }

  stream >> m_indexed;
}


static QDate stringToDate(const QString& s, const QDate& today, bool start) {
  if (s.length() == 8) return QDate::fromString(s, "yyyyMMdd");

  if (s.length() == 4) {
    auto d = QDate::fromString(s, "MMdd");
    d.setDate(today.year(), d.month(), d.day());
    return d;
  }

  if (s.length() == 2) {
    auto d = QDate::fromString(s, "MM");
    if (start) {
      d.setDate(today.year(), d.month(), 1);
    } else {
      int day = 31;
      const int month = d.month();
      while (!d.setDate(today.year(), month, day) && day >= 28) {
        day -= 1;
      }
    }
    return d;
  }

  return QDate();
}

S57::Object::~Object() {
  delete m_geometry;
}

QString S57::Object::name() const {
  auto s = QString("%1-%2-%3").arg(identifier()).arg(classCode()).arg(char(geometry()->type()));
  return s;
}

QVariant S57::Object::attributeValue(quint32 attr) const {
  if (!m_attributes.contains(attr)) {
    // qWarning() << "[Attribute]" << S52::GetAttributeName(attr) << "not present [" << m_geometry->centerLL().print() << "]";
    return QVariant();
  }
  return m_attributes[attr].value();
}

QSet<int> S57::Object::attributeSetValue(quint32 attr) const {
  if (!m_attributes.contains(attr) || m_attributes[attr].type() != Attribute::Type::IntegerList) {
    return QSet<int>();
  }
  auto vs = m_attributes[attr].value().toList();
  QSet<int> rs;
  for (auto v: vs) rs.insert(v.toInt());
  return rs;
}

bool S57::Object::canPaint(const QRectF& viewArea, quint32 scale,
                           const QDate& today, bool onlyViewArea) const {
  if (m_bbox.isValid() && !m_bbox.intersects(viewArea)) {
    // qDebug() << "no intersect" << m_bbox << viewArea << name();
    return false;
  }

  if (onlyViewArea) return true;

  if (m_attributes.contains(scaminIndex)) {
    const quint32 mx = m_attributes[scaminIndex].value().toUInt();
    if (scale > mx) {
      // qDebug() << "scale too small" << scale << mx << name();
      return false;
    }
  }

  if (m_attributes.contains(datstaIndex)) {
    auto d = stringToDate(m_attributes[datstaIndex].value().toString(), today, true);
    if (today < d) return false;
  }

  if (m_attributes.contains(datendIndex)) {
    auto d = stringToDate(m_attributes[datendIndex].value().toString(), today, false);
    if (today > d) return false;
  }

  if (m_attributes.contains(perstaIndex)) {
    auto d = stringToDate(m_attributes[perstaIndex].value().toString(), today, true);
    if (today < d) return false;
  }

  if (m_attributes.contains(perendIndex)) {
    auto d = stringToDate(m_attributes[perendIndex].value().toString(), today, false);
    if (today > d) return false;
  }

  return true;
}


bool S57::Object::canPaint(quint32 scale) const {

  if (m_attributes.contains(scaminIndex)) {
    const quint32 mx = m_attributes[scaminIndex].value().toUInt();
    if (scale > mx) {
      // qDebug() << "scale too small" << scale << mx << name();
      return false;
    }
  }

  return true;
}

double S57::Object::getSafetyContour(double c0) const {
  for (auto c: *m_contours) {
    if (c >= c0) return c;
  }
  return - 15.;
}

void S57::Object::encode(QDataStream &stream, Transform transform) const {
  stream << m_feature_id;
  stream << m_feature_type_code;

  const int Na = m_attributes.size();
  stream << Na;

  for (AttributeIterator it = m_attributes.constBegin(); it != m_attributes.constEnd(); ++it) {
    stream << it.key();
    it.value().encode(stream);
  }
  m_geometry->encode(stream, transform);
  stream << m_bbox;

}

void S57::Object::decode(QDataStream &stream) {
  int Na;
  stream >> Na;

  quint32 key;
  for (int n = 0; n < Na; n++) {
    stream >> key;
    m_attributes[key] = Attribute::Decode(stream);
  }

  m_geometry = Geometry::Base::Decode(stream);

  stream >> m_bbox;
}


S57::Object* S57::Object::Decode(QDataStream &stream) {
  quint32 featureId;
  stream >> featureId;
  quint32 typeCode;
  stream >> typeCode;
  auto obj = new Object(featureId, typeCode);
  obj->decode(stream);

  return obj;
}



