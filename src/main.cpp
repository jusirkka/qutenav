#include <QApplication>
#include <QCommandLineParser>
#include <QStringList>
#include <QDebug>

#include "mainwindow.h"
#include "s52presentation.h"

#include <QSurfaceFormat>
#include <KAboutData>
#include <KLocalizedString>


int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  app.setOrganizationName("Kvanttiapina");
  app.setApplicationName("qopencpn");

  KAboutData aboutData(
        // The program name used internally. (componentName)
        qAppName(),
        // A displayable program name string. (displayName)
        qAppName(),
        // The program version string. (version)
        QStringLiteral("0.030"),
        // Short description of what the app does. (shortDescription)
        i18n("Chart navigator / plotter"),
        // The license this code is released under
        KAboutLicense::GPL,
        // Copyright Statement (copyrightStatement = QString())
        "Jukka Sirkka (c) 2020",
        // Optional text shown in the About box.
        // Can contain any information desired. (otherText)
        i18n("Chart plotter / navigator for Sailfish OS based on OpenCPN."),
        // The program homepage string. (homePageAddress = QString())
        QStringLiteral("https://github.com/jusirkka/qopencpn"),
        // The bug report email address
        // (bugsEmailAddress = QLatin1String("submit@bugs.kde.org")
        QStringLiteral("https://github.com/jusirkka/qopencpn/issues"));

  aboutData.addAuthor(i18n("Jukka Sirkka"),
                      i18n("Codemonkey"),
                      QStringLiteral("jukka.sirkka@iki.fi"),
                      QStringLiteral("https://github.com/jusirkka"));

  KAboutData::setApplicationData(aboutData);

  QCommandLineParser parser;
  aboutData.setupCommandLine(&parser);
  parser.process(app);
  aboutData.processCommandLine(&parser);

  QSurfaceFormat format;
  format.setVersion(4, 6);
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setOption(QSurfaceFormat::DebugContext);
  QSurfaceFormat::setDefaultFormat(format);

  S52::InitPresentation();

  auto mw = new MainWindow();
  mw->show();

  return app.exec();
}
