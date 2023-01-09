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

  const QStringList classNames {"M_QUAL", "M_NSYS", "ADMARE", "CONZNE", "COSARE", "EXEZNE", "FSHZNE",
                                "MIPARE", "NAVLNE", "RADLNE", "RADRNG", "RDOCAL", "SEAARE", "SBDARE",
                                "STSLNE", "TESARE", "CBLSUB"};
  for (const QString& name: classNames) {
    const auto index = S52::FindCIndex(name);
    auto dc = new DisabledClass(index, name, S52::GetClassDescription(index));
    connect(dc, &DisabledClass::enabledChanged, this, &Settings::settingsChanged);
    m_disabledClasses << dc;
  }
}

Settings::~Settings() {
  qDeleteAll(m_textGroups);
  qDeleteAll(m_disabledClasses);
}
