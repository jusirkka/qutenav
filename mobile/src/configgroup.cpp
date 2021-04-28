/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/src/configgroup.cpp
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
#include "configgroup.h"
#include <QStandardPaths>
#include <QSettings>
#include <QDebug>

Conf::ConfigGroup::ConfigGroup(const QString& group, const QString& path)
  : m_group(group)
  , m_dummy(new DummyItem)
{
  const QString loc = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
  m_path = loc + "/" + path;
}

Conf::ConfigGroup::~ConfigGroup() {
  delete m_dummy;
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
    if (it.value() == m_defaults[it.key()]) {
      settings.remove(it.key());
      continue;
    }
    settings.setValue(it.key(), it.value());
  }

  settings.endGroup();
}
