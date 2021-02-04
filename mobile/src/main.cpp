#include <QQmlApplicationEngine>
#include <QGuiApplication>
#include <QtQml>
#include <QDebug>
#include <QQuickView>
#include <QDebug>

#include <sailfishapp.h>
#include "chartdisplay.h"
#include "s52presentation.h"
#include "textmanager.h"
#include "s57chart.h"

Q_IMPORT_PLUGIN(OsencReaderFactory)
Q_IMPORT_PLUGIN(CM93ReaderFactory)

int main(int argc, char *argv[]) {

  Q_INIT_RESOURCE(shaders);

  qmlRegisterType<ChartDisplay>("org.qopencpn", 1, 0, "ChartDisplay");

  QSurfaceFormat format;
  // format.setVersion(4, 6);
  format.setVersion(3, 1);
  format.setRenderableType(QSurfaceFormat::OpenGLES);
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setOption(QSurfaceFormat::DebugContext);
  format.setDepthBufferSize(24);
  format.setStencilBufferSize(8);
  QSurfaceFormat::setDefaultFormat(format);

  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  // QGuiApplication app(argc, argv);
  // Set up QML engine.
  QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
  // remove stutter
  app->setOrganizationName("");
  app->setOrganizationDomain("");

  qRegisterMetaType<TextKey>();
  qRegisterMetaType<GL::GlyphData>();
  qRegisterMetaType<S57Chart*>();
  qRegisterMetaType<WGS84Point>();


  S52::InitPresentation();

  // QQmlApplicationEngine engine;
  // engine.load(QUrl("main.qml"));
  QScopedPointer<QQuickView> view(SailfishApp::createView());

  view->setSource(SailfishApp::pathTo("qml/harbour-qopencpn.qml"));
  view->show();

  // return app.exec();
  return app->exec();
}
