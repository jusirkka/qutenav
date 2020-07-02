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

  // Compare integer lists up to constraint list size
  if (m_type == Type::IntegerList) {
    QList<QVariant> clist = constraint.value().toList();
    QList<QVariant> mlist = m_value.toList();
    if (mlist.size() < clist.size()) return false;
    for (int i = 0; i < clist.size(); i++) {
      if (mlist[i].toInt() != clist[i].toInt()) return false;
    }
    return true;
  }

  return m_value == constraint.value();
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

QString S57::Object::name() const {
  auto s = QString("%1-%2-%3").arg(identifier()).arg(classCode()).arg(char(geometry()->type()));
  return s;
}

QVariant S57::Object::attributeValue(quint32 attr) const {
  if (!m_attributes.contains(attr)) return QVariant();
  return m_attributes[attr].value();
}

bool S57::Object::canPaint(const QRectF& viewArea, quint32 scale, const QDate& today) const {
  if (m_bbox.isValid() && !m_bbox.intersects(viewArea)) {
    // qDebug() << "no intersect" << m_bbox << viewArea << name();
    return false;
  }

  if (m_attributes.contains(scaminIndex)) {
    const quint32 mx = m_attributes[scaminIndex].value().toUInt();
    if (scale > mx) {
      // qDebug() << "scale too big" << scale << mx;
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

