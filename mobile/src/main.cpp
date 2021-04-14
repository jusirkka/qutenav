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

Q_IMPORT_PLUGIN(CM93ReaderFactory)
Q_IMPORT_PLUGIN(S57ReaderFactory)
Q_IMPORT_PLUGIN(OsencReaderFactory)
Q_IMPORT_PLUGIN(OesencReaderFactory)

int main(int argc, char *argv[]) {

  Q_INIT_RESOURCE(shaders);

  qmlRegisterType<ChartDisplay>("org.qopencpn", 1, 0, "ChartDisplay");
  qmlRegisterType<CrossHairs>("org.qopencpn", 1, 0, "CrossHairs");
  qmlRegisterType<Tracker>("org.qopencpn", 1, 0, "Tracker");
  qmlRegisterType<TrackModel>("org.qopencpn", 1, 0, "TrackModel");

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

  QScopedPointer<QQuickView> view(SailfishApp::createView());

  view->rootContext()->setContextProperty("settings", Settings::instance());

  view->setSource(SailfishApp::pathTo("qml/harbour-qopencpn.qml"));
  view->show();

  return app->exec();
}
