#include "chartmanager.h"
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QDebug>
#include "s57chart.h"

ChartManager::ChartManager(QObject *parent) : QObject(parent)
{
//  // TODO: osenc charts from known locations
//  QDir chartDir("/home/jusirkka/.opencpn/SENC");
//  QStringList files = chartDir.entryList(QStringList() << "*.S57",
//                                         QDir::Files | QDir::Readable, QDir::Name);

//  if (files.isEmpty()) {
//    throw ChartFileError(QString("%1 is not a valid chart directory").arg(chartDir.absolutePath()));
//  }
//  qDebug() << files;
//  QString file = files.first();
//  //for (const QString& file: files) {
//    try {
//      auto path = chartDir.absoluteFilePath(file);
//      auto chart = new S57Chart(path, p, this);
//      m_charts << chart;
//    } catch (ChartFileError& e) {
//      qWarning() << "Chart file error:" << e.msg() << ", skipping";
//    }
//  //}

}
