#include "configgroup.h"
#include <QStandardPaths>
#include <QSettings>
#include <QDebug>

Conf::ConfigGroup::ConfigGroup(const QString& group, const QString& path)
  : m_group(group)
{
  const QString loc = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
  m_path = loc + "/" + path;
}

void Conf::ConfigGroup::load() {

  QVariantMap::const_iterator it = m_defaults.cbegin();
  for (; it != m_defaults.end(); ++it) {
    m_values[it.key()] = it.value();
  }

  QSettings settings(m_path, QSettings::IniFormat);
  settings.beginGroup(m_group);

  for (auto& key: settings.childKeys()) {
    if (!m_defaults.contains(key)) continue;
    m_values[key] = settings.value(key);
  }

  settings.endGroup();
}


void Conf::ConfigGroup::save() {

  QSettings settings(m_path, QSettings::IniFormat);
  settings.beginGroup(m_group);

  QVariantMap::const_iterator it = m_values.cbegin();
  for (; it != m_values.end(); ++it) {
    if (it.value() == m_defaults[it.key()]) continue;
    settings.setValue(it.key(), it.value());
  }

  settings.endGroup();
}
