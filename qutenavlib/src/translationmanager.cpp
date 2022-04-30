/* -*- coding: utf-8-unix -*-
 *
 * utils.cpp
 *
 *
 * Copyright (C) 2022 Jukka Sirkka
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

#include <QTranslator>
#include <QStandardPaths>
#include <QLocale>
#include "platform.h"
#include "logging.h"
#include <QDir>
#include <QDateTime>
#include "translationmanager.h"


TranslationManager* TranslationManager::instance() {
  static TranslationManager* mgr = new TranslationManager;
  return mgr;
}

TranslationManager::TranslationManager():
  m_locale(defaultLocale)
{}

void TranslationManager::loadTranslation(QTranslator& translator) {

  QStringList langs = QLocale().uiLanguages();
  // append fallback
  if (!langs.contains(defaultLocale)) {
    langs.append(defaultLocale);
  }

  QMap<QString, QStringList> trs;
  for (const QString& lang: langs) {
    for (const QString& loc: QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
      trs[lang] << QString("%1/%2/translations/id_%3").arg(loc).arg(baseAppName()).arg(lang);
    }
  }

  bool done = false;
  for (const QString& lang: langs) {
    const auto paths = trs[lang];
    for (const QString& path: paths) {
      qCDebug(CDPY) << "checking" << path;
      done = translator.load(path);
      if (done) {
        m_locale = lang;
        break;
      }
    }
    if (done) break;
  }

  if (translator.isEmpty()) {
    qCWarning(CDPY) << "No translations: UI might be difficult to fathom";
  }
}

