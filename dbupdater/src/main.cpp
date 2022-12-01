/* -*- coding: utf-8-unix -*-
 *
 * File: src/dbupdater.cpp
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
#include <QCoreApplication>
#include <QDebug>
#include <QtPlugin>
#include <QtDBus/QDBusConnection>

#include "dbupdater_adaptor.h"
#include "s52names.h"
#include "chartdatabase.h"
#include "state.h"

Q_IMPORT_PLUGIN(CM93ReaderFactory)
Q_IMPORT_PLUGIN(S57ReaderFactory)
Q_IMPORT_PLUGIN(OsencReaderFactory)
Q_IMPORT_PLUGIN(OesencReaderFactory)
Q_IMPORT_PLUGIN(OesuReaderFactory)

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);

  qRegisterMetaType<State::OutlineData>();
  qRegisterMetaType<S57ChartOutline>();

  S52::InitNames();
  auto updater = new Updater;
  new UpdaterAdaptor(updater);

  QDBusConnection conn = QDBusConnection::sessionBus();
  auto ok = conn.registerObject("/Updater",
                                updater,
                                QDBusConnection::ExportAdaptors | QDBusConnection::ExportAllContents);
  if (!ok) {
    qWarning() << "Cannot register to session dbus. Is" << qAppName() << "already running?";
    return 255;
  }

  conn.registerService("net.kvanttiapina.qutenav");

  ChartDatabase::createTables();

  return app.exec();
}
