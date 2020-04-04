#include <QApplication>
#include <QCommandLineParser>
#include <QStringList>
#include <QDebug>

#include <gdal/ogrsf_frmts.h>
#include "mainwindow.h"
#include "globe.h"
#include "outliner.h"

#include <QSurfaceFormat>

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  app.setOrganizationName("Kvanttiapina");
  app.setApplicationName("qopencpn");
  app.setApplicationVersion("0.026");

  QCommandLineParser parser;
  parser.setApplicationDescription("Chart plotter / navigator based on OpenCPN for Sailfish OS");
  parser.addHelpOption();
  parser.addVersionOption();


  parser.process(app);

  GDALAllRegister();

  QSurfaceFormat format;
  format.setVersion(4, 5);
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setOption(QSurfaceFormat::DebugContext);
  QSurfaceFormat::setDefaultFormat(format);

  MainWindow mw;
  mw.show();

  return app.exec();

}
