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

bool TextGroup::enabled() const {
  return Conf::MarinerParams::TextGrouping().contains(m_group);
}

void TextGroup::setEnabled(bool v) {
  auto values = Conf::MarinerParams::TextGrouping();
  if (values.contains(m_group) && v) return;
  if (!values.contains(m_group) && !v) return;

  if (values.contains(m_group)) {
    values.removeOne(m_group);
  } else {
    values.append(m_group);
  }
  Conf::MarinerParams::setTextGrouping(values);
  emit enabledChanged();
}

Settings* Settings::instance() {
  static Settings* s = new Settings();
  return s;
}

Settings::Settings(QObject *parent)
  : QObject(parent)
{
  for (auto it = texts.cbegin(); it != texts.cend(); ++it) {
    if (descriptions.contains(it.key())) {
      m_textGroups << new TextGroup(it.key(), true, qtTrId(it.value()), qtTrId(descriptions[it.key()]));
    } else {
      m_textGroups << new TextGroup(it.key(), true, qtTrId(it.value()));
    }
  }

  for (auto obj: m_textGroups) {
    auto group = qobject_cast<TextGroup*>(obj);
    connect(group, &TextGroup::enabledChanged, this, &Settings::settingsChanged);
  }
}


float Settings::displayLengthScaling() const {
  const float y0 = 6.2 / nominal_dpmm;
  const float y1 = 9.0 / nominal_dpmm;
  const float t = (dots_per_mm_y() - dpmm0) / delta_dpmm1;
  return y1 * t + y0 * (1 - t);
}

float Settings::displayTextSizeScaling() const {
  const float y0 = 3.5;
  const float y1 = 4.2;
  const float t = (dots_per_mm_y() - dpmm0) / delta_dpmm1;
  return (y1 * t + y0 * (1 - t));
}

float Settings::displayLineWidthScaling() const {
  const float y0 = .7;
  const float y1 = .3;
  const float t = (dots_per_mm_y() - dpmm0) / delta_dpmm1;
  return y1 * t + y0 * (1 - t);
}

float Settings::displayRasterSymbolScaling() const {
  const float y0 = 1.;
  const float y1 = .5;
  const float t = (dots_per_mm_y() - dpmm0) / delta_dpmm1;
  return y1 * t + y0 * (1 - t);
}

Settings::~Settings() {
  qDeleteAll(m_textGroups);
}
