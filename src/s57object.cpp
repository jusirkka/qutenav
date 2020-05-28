#include "s57object.h"



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
