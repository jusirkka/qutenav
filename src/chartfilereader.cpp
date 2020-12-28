#include "chartfilereader.h"
#include "osencreader.h"
#include "cm93reader.h"

QVector<ChartFileReader*> ChartFileReader::readers() {
  static QVector<ChartFileReader*> all{new OsencReader(), new CM93Reader()};
  return all;
}

ChartFileReader* ChartFileReader::reader(const QString& name) {
  for (ChartFileReader* c: readers()) {
    if (c->name() == name) return c;
  }
  return nullptr;
}

QStringList ChartFileReader::names() {
  QStringList ns;
  for (ChartFileReader* c: readers()) {
    ns << c->name();
  }
  return ns;
}

