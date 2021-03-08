#include <QCoreApplication>
#include <QDebug>
#include <QtPlugin>
#include <QtDBus/QDBusConnection>

#include "dbupdater_adaptor.h"
#include "s52names.h"

Q_IMPORT_PLUGIN(CM93ReaderFactory)
Q_IMPORT_PLUGIN(S57ReaderFactory)
Q_IMPORT_PLUGIN(OsencReaderFactory)
Q_IMPORT_PLUGIN(OesencReaderFactory)

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);

  S52::InitNames();
  auto updater = new Updater;
  new UpdaterAdaptor(updater);

  QDBusConnection conn = QDBusConnection::sessionBus();
  conn.registerObject("/Updater",
                      updater,
                      QDBusConnection::ExportAdaptors | QDBusConnection::ExportAllContents);
  conn.registerService("net.kvanttiapina.qopencpn");

  return app.exec();
}
