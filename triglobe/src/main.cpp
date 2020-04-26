#include "sdfunction.h"
#include "triangulator.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <fenv.h>
#include <QDir>
#include <gdal/ogrsf_frmts.h>
#include <QDebug>

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);

  feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW);

  QCommandLineParser parser;

  QCommandLineOption errOpt(QStringList() << "e" << "edge-error",
                            "Relative edge error estimate",
                            "float");
  parser.addOption(errOpt);
  QCommandLineOption expOpt(QStringList() << "n" << "max-expands",
                            "Number of expands",
                            "int");
  parser.addOption(expOpt);

  QCommandLineOption joinOpt(QStringList() << "j" << "max-joins",
                            "Number of joins",
                            "int");
  parser.addOption(joinOpt);

  QCommandLineOption splitOpt(QStringList() << "s" << "max-splits",
                            "Number of splits",
                            "int");
  parser.addOption(splitOpt);

  parser.process(app);

  bool ok;
  scalar err = parser.value(errOpt).toDouble(&ok);
  if (!ok) {
    err = 0.005;
  }
  uint n = parser.value(expOpt).toInt(&ok);
  if (!ok) {
    n = -1;
  }
  uint s = parser.value(splitOpt).toInt(&ok);
  if (!ok) {
    s = -1;
  }
  uint j = parser.value(joinOpt).toInt(&ok);
  if (!ok) {
    j = -1;
  }

  GDALAllRegister();

  QDir chartDir("/home/jusirkka/share/gshhg/GSHHS_shp/c");
  QStringList charts = chartDir.entryList(QStringList() << "*_L?.shp",
                                          QDir::Files | QDir::Readable, QDir::Name);

  if (charts.isEmpty()) {
    qFatal("%s is not a valid chart directory", chartDir.absolutePath().toUtf8().data());
  }
  qDebug() << charts;

  auto globe = new SD::Sphere;
  Triangulator t(globe, err, n, s, j);

  for (const QString& chart: charts) {
    auto path = chartDir.absoluteFilePath(chart);
    auto world = static_cast<GDALDataset*>(GDALOpenEx(path.toUtf8(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr));
    if (world == nullptr) continue;

    OGRLayer* layer = world->GetLayer(0);
    auto feature = layer->GetNextFeature();
    auto poly = dynamic_cast<const OGRPolygon*>(feature->GetGeomFieldRef(0));
    Triangulator::Mesh front;
    for (OGRPoint p: poly->getExteriorRing()) {
      scalar lng = p.getX() * SD::Function::RADS_PER_DEG;
      scalar lat = p.getY() * SD::Function::RADS_PER_DEG;
      front.append(vec3(cos(lng) * cos(lat),
                      sin(lng) * cos(lat),
                      sin(lat)));
    }

    t.addFront(front);
    t.triangulate();
    break;
  }

  QFile f("globe.obj");
  f.open(QIODevice::WriteOnly);
  t.write(f);
  f.close();
  delete globe;

  return 0;
}
