/* -*- coding: utf-8-unix -*-
 *
 * File: src/settings.cpp
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
#include "settings.h"
#include <QDebug>
#include "platform.h"
#include <QMapIterator>
#include "s52names.h"


Settings* Settings::instance() {
  static Settings* s = new Settings();
  return s;
}

Settings::Settings(QObject *parent)
  : QObject(parent)
{
  for (auto it = texts.cbegin(); it != texts.cend(); ++it) {
    TextGroup* group;
    if (descriptions.contains(it.key())) {
      group = new TextGroup(it.key(), qtTrId(it.value()), qtTrId(descriptions[it.key()]));
    } else {
      group = new TextGroup(it.key(), qtTrId(it.value()));
    }
    connect(group, &TextGroup::enabledChanged, this, &Settings::settingsChanged);
    m_textGroups << group;
  }

  const QStringList optionalClassNames {"M_QUAL", "M_NSYS", "ADMARE", "CONZNE", "COSARE",
                                        "EXEZNE", "FSHZNE", "MIPARE", "NAVLNE", "RADLNE",
                                        "RADRNG", "RDOCAL", "SEAARE", "SBDARE", "STSLNE",
                                        "TESARE", "CBLSUB"};
  for (const QString& name: optionalClassNames) {
    const auto index = S52::FindCIndex(name);
    auto dc = new OptionalClass(index, name, S52::GetClassDescription(index));
    connect(dc, &OptionalClass::enabledChanged, this, &Settings::settingsChanged);
    m_optionalClasses << dc;
  }
  std::sort(m_optionalClasses.begin(), m_optionalClasses.end(), [] (const QObject* o1, const QObject*  o2) {
    return qobject_cast<const OptionalClass*>(o1)->text() < qobject_cast<const OptionalClass*>(o2)->text();
  });

  const QStringList scaminClassNames {"DEPCNT", "RECTRC"};
  for (const QString& name: scaminClassNames) {
    const auto index = S52::FindCIndex(name);
    auto sc = new ScaminItem(index, name, S52::GetClassDescription(index));
    connect(sc, &ScaminItem::valueChanged, this, &Settings::settingsChanged);
    m_scaminClasses << sc;
  }
  std::sort(m_scaminClasses.begin(), m_scaminClasses.end(), [] (const QObject* o1, const QObject*  o2) {
    return qobject_cast<const ScaminItem*>(o1)->text() < qobject_cast<const ScaminItem*>(o2)->text();
  });

}

Settings::~Settings() {
  qDeleteAll(m_textGroups);
  qDeleteAll(m_optionalClasses);
  qDeleteAll(m_scaminClasses);
}
