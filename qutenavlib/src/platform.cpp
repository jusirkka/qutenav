/* -*- coding: utf-8-unix -*-
 *
 * platform.cpp
 *
 * Created: 13/05/2021 2021 by Jukka Sirkka
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

#include "platform.h"
#include <QGuiApplication>
#include <QScreen>
#include <QTranslator>
#include <QStandardPaths>
#include <QLocale>
#include "logging.h"

float dots_per_mm_x() {
  static float v = QGuiApplication::primaryScreen()->physicalDotsPerInchX() / 25.4;
  return v;
}

float dots_per_mm_y() {
  static float v = QGuiApplication::primaryScreen()->physicalDotsPerInchY() / 25.4;
  return v;
}

float dots_per_inch_x() {
  static float v = QGuiApplication::primaryScreen()->physicalDotsPerInchX();
  return v;
}

float dots_per_inch_y() {
  static float v = QGuiApplication::primaryScreen()->physicalDotsPerInchY();
  return v;
}

const QString& baseAppName() {
  // qutenav, harbour-qutenav, qutenav_dbupdater or harbour_qutenav_updater ->
  // qutenav or harbour-qutenav
  static QString name = qAppName().replace("_dbupdater", "").replace("_", "-");
  return name;
}


void loadTranslation(QTranslator& translator) {

  QStringList langs = QLocale().uiLanguages();
  // append fallback
  if (!langs.contains("en")) {
    langs.append("en");
  }

  QStringList trs;
  for (const QString& lang: langs) {
    for (const QString& loc: QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
      trs << QString("%1/%2/translations/id_%3").arg(loc).arg(baseAppName()).arg(lang);
    }
  }

  for (const QString& tr: trs) {
    qCDebug(CDPY) << "checking" << tr;
    if (translator.load(tr)) {
      break;
    }
  }

  if (translator.isEmpty()) {
    qCWarning(CDPY) << "No translations: UI might be difficult to read";
  }
}
