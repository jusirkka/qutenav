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

Q_IMPORT_PLUGIN(CM93ReaderFactory)
Q_IMPORT_PLUGIN(S57ReaderFactory)
Q_IMPORT_PLUGIN(OsencReaderFactory)
Q_IMPORT_PLUGIN(OesencReaderFactory)

QOpenGLContext *qt_gl_global_share_context();
void qt_gl_set_global_share_context(QOpenGLContext *context);

int main(int argc, char *argv[]) {

  Q_INIT_RESOURCE(shaders);

  qmlRegisterType<ChartDisplay>("org.qopencpn", 1, 0, "ChartDisplay");
  qmlRegisterType<CrossHairs>("org.qopencpn", 1, 0, "CrossHairs");
  qmlRegisterType<Tracker>("org.qopencpn", 1, 0, "Tracker");
  qmlRegisterType<Router>("org.qopencpn", 1, 0, "Router");
  qmlRegisterType<TrackModel>("org.qopencpn", 1, 0, "TrackModel");
  qmlRegisterType<RouteModel>("org.qopencpn", 1, 0, "RouteModel");

  QSurfaceFormat format;
  format.setVersion(3, 2);
  format.setRenderableType(QSurfaceFormat::OpenGLES);
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setOption(QSurfaceFormat::DebugContext);
  format.setDepthBufferSize(24);
  format.setStencilBufferSize(8);
  QSurfaceFormat::setDefaultFormat(format);

  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  // Set up QML engine.
  QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));

  // HACK: workaround for SailfishApp::application returning cached app which
  // was created (presumably) before setting AA_ShareOpenGLContexts. As a result,
  // neither global context is created nor any contexts are shared.
  if (qt_gl_global_share_context() == nullptr) {
    qDebug() << "HACK: Creating global opengl context";
    QOpenGLContext *ctx = new QOpenGLContext;
    ctx->setFormat(QSurfaceFormat::defaultFormat());
    ctx->create();
    qt_gl_set_global_share_context(ctx);
  }

  // remove stutter
  app->setOrganizationName("");
  app->setOrganizationDomain("");

  qRegisterMetaType<TextKey>();
  qRegisterMetaType<GL::GlyphData>();
  qRegisterMetaType<S57Chart*>();
  qRegisterMetaType<WGS84Point>();
  qRegisterMetaType<S57::InfoType>();
  qRegisterMetaType<ChartData>();

  S52::InitPresentation();

  TrackDatabase::createTables();
  RouteDatabase::createTables();

  QScopedPointer<QQuickView> view(SailfishApp::createView());

  view->rootContext()->setContextProperty("settings", Settings::instance());

  view->setSource(SailfishApp::pathTo("qml/harbour-qopencpn.qml"));
  view->show();

  return app->exec();
}
