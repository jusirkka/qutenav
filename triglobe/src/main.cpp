#include "sdfunction.h"
#include "triangulator.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <fenv.h>
#include <QDir>
#include <gdal/ogrsf_frmts.h>
#include <QDebug>
#include <QRegularExpression>
#include <QStandardPaths>

static int indexFromChartName(const QString &name);

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

  QCommandLineOption cOpt(QStringList() << "c" << "coarseness",
                            "Coarseness subdirectory",
                            "string");
  parser.addOption(cOpt);

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

  QString c = parser.value(cOpt);
  if (c.isEmpty()) {
    c = "c";
  }


  GDALAllRegister();

  // FIXME: search standard paths
  QDir chartDir(QString("/home/jusirkka/share/gshhg/GSHHS_shp/%1").arg(c));
  QStringList charts = chartDir.entryList(QStringList() << "*_L?.shp",
                                          QDir::Files | QDir::Readable, QDir::Name);

  if (charts.isEmpty()) {
    qFatal("%s is not a valid chart directory", chartDir.absolutePath().toUtf8().data());
  }
  qDebug() << charts;

  // output path
  QString opath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
  opath = QString("%1/qopencpn/GSHHS/%2").arg(opath).arg(c);
  if (!QDir(opath).exists()) {
    QDir().mkpath(opath);
  }

  auto globe = new SD::Sphere;

  Triangulator t(globe, err, n, s, j);
  t.seedAndTriangulate();
  QFile f(QString("%1/globe_l0.obj").arg(opath));
  f.open(QIODevice::WriteOnly);
  t.write(f);
  f.close();

  for (const QString& chart: charts) {
    auto path = chartDir.absoluteFilePath(chart);
    auto world = static_cast<GDALDataset*>(GDALOpenEx(path.toUtf8(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr));
    if (world == nullptr) continue;

    Triangulator t(globe, err, n, s, j);
    Triangulator t0(globe, err, n, s, j);

    OGRLayer* layer = world->GetLayer(0);
    auto feature = layer->GetNextFeature();
    while (feature != nullptr) {
      for (int i = 0; i < feature->GetGeomFieldCount(); i++) {
        auto poly = dynamic_cast<const OGRPolygon*>(feature->GetGeomFieldRef(i));
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
        t0.addFront(front);
      }
      feature = layer->GetNextFeature();
    }

    QFile f(QString("%1/globe_l%2.obj").arg(opath).arg(indexFromChartName(chart)));
    f.open(QIODevice::WriteOnly);
    t.write(f);
    f.close();

    QFile f0(QString("%1/globe_shape_l%2.obj").arg(opath).arg(indexFromChartName(chart)));
    f0.open(QIODevice::WriteOnly);
    t0.write(f0);
    f0.close();
  }

  delete globe;
  return 0;
}

static int indexFromChartName(const QString &name) {
  static const QRegularExpression re(".*_L(\\d)\\.shp$");
  QRegularExpressionMatch match = re.match(name);
  if (!match.hasMatch()) {
    qFatal("Error parsing chart name %s", name.toUtf8().data());
  }
  return match.captured(1).toInt();
}

