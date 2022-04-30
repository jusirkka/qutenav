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
#include <QDebug>
#include <QQuickView>
#include <QDebug>

#include <sailfishapp.h>
#include "chartdisplay.h"
#include "crosshairs.h"
#include "s52presentation.h"
#include "textmanager.h"
#include "s57chart.h"
#include "settings.h"
#include "chartupdater.h"
#include "tracker.h"
#include "trackmodel.h"
#include "router.h"
#include "routemodel.h"
#include "routedatabase.h"
#include "chartdatabase.h"
#include "s57imageprovider.h"
#include "units.h"
#include "translationmanager.h"

Q_IMPORT_PLUGIN(CM93ReaderFactory)
Q_IMPORT_PLUGIN(S57ReaderFactory)
Q_IMPORT_PLUGIN(OsencReaderFactory)
Q_IMPORT_PLUGIN(OesencReaderFactory)

int main(int argc, char *argv[]) {

  Q_INIT_RESOURCE(shaders_opengl_es);

  qmlRegisterType<ChartDisplay>("org.qutenav", 1, 0, "ChartDisplay");
  qmlRegisterType<CrossHairs>("org.qutenav", 1, 0, "CrossHairs");
  qmlRegisterType<Tracker>("org.qutenav", 1, 0, "Tracker");
  qmlRegisterType<Router>("org.qutenav", 1, 0, "Router");
  qmlRegisterType<TrackModel>("org.qutenav", 1, 0, "TrackModel");
  qmlRegisterType<RouteModel>("org.qutenav", 1, 0, "RouteModel");

  QSurfaceFormat format;
  format.setVersion(3, 2);
  format.setRenderableType(QSurfaceFormat::OpenGLES);
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setOption(QSurfaceFormat::DebugContext);
  format.setDepthBufferSize(24);
  format.setStencilBufferSize(8);
  QSurfaceFormat::setDefaultFormat(format);

  // Set up QML engine.
  QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));

  QTranslator tr;
  TranslationManager::instance()->loadTranslation(tr);
  app->installTranslator(&tr);

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

  S52::InitPresentation();

  TrackDatabase::createTables();
  RouteDatabase::createTables();
  ChartDatabase::createTables();

  QScopedPointer<QQuickView> view(SailfishApp::createView());

  view->rootContext()->setContextProperty("settings", Settings::instance());
  view->rootContext()->setContextProperty("units", Units::Manager::instance());
  view->engine()->addImageProvider(QLatin1String("s57"), new S57::ImageProvider);

  view->setSource(SailfishApp::pathTo("qml/harbour-qutenav.qml"));
  view->show();

  return app->exec();
}
