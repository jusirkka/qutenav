#include "s57chart.h"
#include "drawable.h"
#include <QFile>
#include <QDataStream>
#include <QVector>
#include <functional>

using Buffer = QVector<char>;
using HandlerFunc = std::function<bool (const Buffer&)>;

struct Handler {
  Handler(HandlerFunc f): func(std::move(f)) {}
  HandlerFunc func;
  ~Handler() = default;
};

S57Chart::S57Chart(const QString& path, QObject *parent)
  : QObject(parent)
{

  QFile file(path);
  if (!file.open(QFile::ReadOnly)) {
    throw ChartFileError(QString("Cannot open %1 for reading").arg(path));
  }
  QDataStream stream(&file);

  Buffer buffer;

  buffer.resize(sizeof(OSENC_Record_Base));
  if (stream.readRawData(buffer.data(), buffer.size()) == -1) {
    throw ChartFileError(QString("Error reading %1 bytes from %2").arg(buffer.size()).arg(path));
  }

  //  For identification purposes, the very first record must be the OSENC Version Number Record
  auto record = reinterpret_cast<OSENC_Record_Base*>(buffer.data());

  // Check Record
  if (record->record_type != SencRecordType::HEADER_SENC_VERSION){
    throw ChartFileError(QString("%1 is not a supported senc file").arg(path));
  }

  //  This is the correct record type (OSENC Version Number Record), so read it
  buffer.resize(record->record_length - sizeof(OSENC_Record_Base));
  if (stream.readRawData(buffer.data(), buffer.size()) == -1) {
    throw ChartFileError(QString("Error reading %1 bytes from %2").arg(buffer.size()).arg(path));
  }
  auto p16 = reinterpret_cast<quint16*>(buffer.data());
  qDebug() << "senc version =" << *p16;


  const QMap<SencRecordType, Handler*> handlers {
    {SencRecordType::HEADER_CELL_NAME, new Handler([] (const Buffer& b) {
        qDebug() << "cell name" << QString::fromUtf8(b.constData());
        return true;
      })
    },
    {SencRecordType::HEADER_CELL_PUBLISHDATE, new Handler([] (const Buffer& b) {
        qDebug() << "cell publishdate" << QString::fromUtf8(b.constData());
        return true;
      })
    },
    {SencRecordType::HEADER_CELL_UPDATEDATE, new Handler([] (const Buffer& b) {
        qDebug() << "cell updatedate" << QString::fromUtf8(b.constData());
        return true;
      })
    },
    {SencRecordType::HEADER_CELL_EDITION, new Handler([] (const Buffer& b) {
        const quint16* p16 = reinterpret_cast<const quint16*>(b.constData());
        qDebug() << "cell edition" << *p16;
        return true;
      })
    },
    {SencRecordType::HEADER_CELL_UPDATE, new Handler([] (const Buffer& b) {
        const quint16* p16 = reinterpret_cast<const quint16*>(b.constData());
        qDebug() << "cell update" << *p16;
        return true;
      })
    },
    {SencRecordType::HEADER_CELL_NATIVESCALE, new Handler([] (const Buffer& b) {
        const quint32* p32 = reinterpret_cast<const quint32*>(b.constData());
        qDebug() << "cell nativescale" << *p32;
        return true;
      })
    },
    {SencRecordType::HEADER_CELL_SENCCREATEDATE, new Handler([] (const Buffer& b) {
        qDebug() << "cell senccreatedate" << QString::fromUtf8(b.constData());
        return true;
      })
    },
    {SencRecordType::CELL_EXTENT_RECORD, new Handler([this] (const Buffer& b) {
        qDebug() << "cell extent record";
        const OSENC_EXTENT_Record_Payload* p = reinterpret_cast<const OSENC_EXTENT_Record_Payload*>(b.constData());
        Extent::EightFloater points;
        points.append(p->extent_sw_lon);
        points.append(p->extent_sw_lat);
        points.append(p->extent_se_lon);
        points.append(p->extent_se_lat);
        points.append(p->extent_ne_lon);
        points.append(p->extent_ne_lat);
        points.append(p->extent_nw_lon);
        points.append(p->extent_nw_lat);

        m_extent = Extent(points);
        return true;
      })
    }
  };

  bool done = false;

  while (!done) {

    buffer.resize(sizeof(OSENC_Record_Base));
    if (stream.readRawData(buffer.data(), buffer.size()) == -1) {
      done = true;
      continue;
    }
    record = reinterpret_cast<OSENC_Record_Base*>(buffer.data());
    // copy, record_type will be overwritten later
    SencRecordType rec_type = record->record_type;
    if (!handlers.contains(rec_type)) {
      done = true;
      continue;
    }
    buffer.resize(record->record_length - sizeof(OSENC_Record_Base));
    if (stream.readRawData(buffer.data(), buffer.size()) == -1) {
      done = true;
      continue;
    }
    if (!(handlers[rec_type])->func(buffer)) {
      done = true;
      continue;
    }
  }
  qDeleteAll(handlers);
}

const Extent& S57Chart::extent() const {
  return m_extent;
}

