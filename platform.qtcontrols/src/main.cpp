/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/src/main.cpp
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
#include <QQmlApplicationEngine>
#include <QGuiApplication>
#include <QtQml>
#include <QQuickView>

#include "chartdisplay.h"
#include "crosshairs.h"
#include "s52presentation.h"
#include "textmanager.h"
#include "s57chart.h"
#include "settings.h"
#include "units.h"
#include "chartupdater.h"
#include "tracker.h"
#include "trackmodel.h"
#include "router.h"
#include "routemodel.h"
#include "chartindicator.h"
#include "routedatabase.h"
#include "chartdatabase.h"
#include "s57imageprovider.h"
#include "translationmanager.h"

Q_IMPORT_PLUGIN(CM93ReaderFactory)
Q_IMPORT_PLUGIN(S57ReaderFactory)
Q_IMPORT_PLUGIN(OsencReaderFactory)
Q_IMPORT_PLUGIN(OesencReaderFactory)
Q_IMPORT_PLUGIN(GSHHSReaderFactory)

int main(int argc, char *argv[]) {

  Q_INIT_RESOURCE(qml_qtcontrols);
  Q_INIT_RESOURCE(shaders_opengl_desktop);

  qmlRegisterType<ChartDisplay>("org.qutenav", 1, 0, "ChartDisplay");
  qmlRegisterType<CrossHairs>("org.qutenav", 1, 0, "CrossHairs");
  qmlRegisterType<Tracker>("org.qutenav", 1, 0, "Tracker");
  qmlRegisterType<Router>("org.qutenav", 1, 0, "Router");
  qmlRegisterType<TrackModel>("org.qutenav", 1, 0, "TrackModel");
  qmlRegisterType<RouteModel>("org.qutenav", 1, 0, "RouteModel");
  qmlRegisterType<ChartIndicator>("org.qutenav", 1, 0, "ChartIndicator");

  QSurfaceFormat format;
  format.setVersion(4, 6);
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setOption(QSurfaceFormat::DebugContext);
  format.setDepthBufferSize(24);
  format.setStencilBufferSize(8);
  QSurfaceFormat::setDefaultFormat(format);

  QScopedPointer<QGuiApplication> app(new QGuiApplication(argc,argv));
  // remove stutter
  app->setOrganizationName("");
  app->setOrganizationDomain("");

  qRegisterMetaType<TextKey>();
  qRegisterMetaType<GL::GlyphData>();
  qRegisterMetaType<GL::ChartProxy*>();
  qRegisterMetaType<S57Chart*>();
  qRegisterMetaType<WGS84Point>();
  qRegisterMetaType<S57::InfoType>();
  qRegisterMetaType<ChartData>();

  QTranslator tr;
  TranslationManager::instance()->loadTranslation(tr);
  app->installTranslator(&tr);

  S52::InitPresentation();

  TrackDatabase::createTables();
  RouteDatabase::createTables();
  ChartDatabase::createTables();

  // Set up QML engine.
  QQmlApplicationEngine engine;
  engine.addImageProvider(QLatin1String("s57"), new S57::ImageProvider);
  engine.rootContext()->setContextProperty("settings", Settings::instance());
  engine.rootContext()->setContextProperty("units", Units::Manager::instance());
  engine.load(QUrl("qrc:///qutenav.qml"));

  return app->exec();
}
