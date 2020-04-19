#include "sdfunction.h"
#include "triangulator.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <fenv.h>

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);

  feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW);



  QCommandLineParser parser;

  QCommandLineOption errOpt(QStringList() << "e" << "edge-error-estimate",
                            "Relative edge error estimate",
                            "float");
  parser.addOption(errOpt);
  QCommandLineOption expOpt(QStringList() << "n" << "max-expands",
                            "Number of expands",
                            "int");
  parser.addOption(expOpt);

  parser.process(app);

  bool ok;
  scalar err = parser.value(errOpt).toDouble(&ok);
  if (!ok) {
    err = 0.005;
  }
  uint n = parser.value(expOpt).toInt(&ok);
  if (!ok) {
    n = 0;
  }

  // circle -> translate -> revolve
  auto torus = new SD::Revolve(new SD::Translate2D(new SD::Circle, vec2(2., 0.)));

  // Triangulator t(new SD::Shift(new SD::Scale(new SD::Lift(new SD::Circle, 3), 3), .2), err, n);
  Triangulator t(torus, err, n);
  t.triangulate();

  QFile f("rivet.obj");
  f.open(QIODevice::WriteOnly);
  t.write(f);
  f.close();
  delete torus;

  return 0;
}
