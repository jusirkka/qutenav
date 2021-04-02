#pragma once

#include <QVariantMap>
#include <QDebug>

#define CONF_DECL(name, Name, key, Type, toType) \
  static void set ## Name(Type v) {\
    self()->m_values[#key] = v;\
  } \
  static Type name() {\
    return self()->m_values[#key].toType(); \
  }

namespace Conf {

class DummyItem {
public:
  QVariant getDefault() const {return QVariant();}
};

class ConfigGroup {
public:

  void save();

  DummyItem* findItem(const QString&) const {return m_dummy;}

  virtual ~ConfigGroup();

protected:

  using QStringMap = QMap<QString, QString>;

  ConfigGroup(const QString& group, const QString& file);
  void load();

  QVariantMap m_values;
  QVariantMap m_defaults;
  QString m_group;
  QString m_path;

  DummyItem* m_dummy;

};

}
