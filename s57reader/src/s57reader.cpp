/* -*- coding: utf-8-unix -*-
 *
 * File: s57reader/src/s57reader.cpp
 *
 * Copyright (C) 2021 Jukka Sirkka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "s57reader.h"
#include <QFile>
#include <QDataStream>
#include <functional>
#include <QDate>
#include "s52names.h"
#include "record.h"
#include <QDir>
#include <QFileInfo>
#include "logging.h"

const GeoProjection* S57Reader::geoprojection() const {
  return m_proj;
}

S57Reader::S57Reader(const QString& name)
  : ChartFileReader(name)
  , m_proj(GeoProjection::CreateProjection("SimpleMercator"))
{}



struct FieldInfo {
  FieldInfo() = default;
  FieldInfo(const QString& t, quint32 l, quint32 p)
    : tag(t)
    , len(l)
    , pos(p) {}

  QString tag;
  quint32 len;
  quint32 pos;

};

struct FieldData {
  FieldData() = default;
  FieldData(const QString& t, const QByteArray& b)
    : tag(t)
    , bytes(b) {}

  QString tag;
  QByteArray bytes;
};

using LexLevel = S57::Record::LexLevel::palette;


class FieldSource {

  static const quint8 fieldControlLen = 9;
  static const quint8 fieldTagFieldLen = 4;
  static const quint8 leaderLen = 24;
  static const quint8 fieldTerminator = 0x1e;
  static const quint8 unitTerminator = 0x1f;

public:
  FieldSource(QFile* file, const QString& path);
  S57::Record* nextRecord();
  void setLexicalLevels(LexLevel alevel, LexLevel nlevel) {
    m_attfLevel = alevel;
    m_natfLevel = nlevel;
  }

private:

  using FieldInfoVector = QVector<FieldInfo>;
  using FieldDataVector = QVector<FieldData>;

  FieldInfoVector readDirectory(quint32 hlen, quint8 flen, quint8 plen);
  QDataStream m_stream;
  QStringList m_tags;
  LexLevel m_attfLevel;
  LexLevel m_natfLevel;

};

S57::Record* FieldSource::nextRecord() {

  if (m_stream.atEnd()) {
    return nullptr;
  }
  // read leader
  m_stream.skipRawData(12); // reclen(5), " D     "
  auto headerlen = read_integer<quint32>(m_stream, 5);
  m_stream.skipRawData(3); // "   "
  auto fieldLenFieldlen = read_integer<quint8>(m_stream, 1);
  auto fieldPosFieldlen = read_integer<quint8>(m_stream, 1);
  m_stream.skipRawData(2); // "04"
  // read directory
  FieldInfoVector fieldInfo = readDirectory(headerlen, fieldLenFieldlen, fieldPosFieldlen);
  // decode records
  // skip ISO/IEC 8211 Record Identifier
  FieldInfo f0 = fieldInfo.takeFirst();
  m_stream.skipRawData(f0.len);
  S57::Record* head = nullptr;
  S57::Record* p = nullptr;
  do {
    FieldInfo f = fieldInfo.takeFirst();
    // qCDebug(CENC) << "block" << f.tag << f.len - 1 << fieldInfo.size();
    QByteArray block(f.len - 1, '0');
    m_stream.readRawData(block.data(), f.len - 1);
    quint8 term;
    m_stream >> term;
    // only NATF can have UCS-2 text
    if (f.tag == "NATF" && term == 0x00) {
      term = block.at(block.size() - 1);
      block.chop(1);
    }
    Q_ASSERT(term == fieldTerminator);
    auto rec = S57::Record::Create(f.tag, block, m_attfLevel, m_natfLevel);
    if (!head) {
      head = rec;
      p = head;
    } else {
      p->next = rec;
      p = p->next;
    }
  } while (!fieldInfo.isEmpty());

  return head;
}

QVector<FieldInfo> FieldSource::readDirectory(quint32 hlen, quint8 flen, quint8 plen) {
  int numItems = (hlen - leaderLen - 1) / (fieldTagFieldLen + flen + plen);
  QVector<FieldInfo> fieldInfo;
  while (numItems > 0) {
    auto t = read_string(m_stream, fieldTagFieldLen);
    auto l = read_integer<quint32>(m_stream, flen);
    auto p = read_integer<quint32>(m_stream, plen);
    fieldInfo.append(FieldInfo(t, l, p));
    numItems--;
  }
  quint8 term;
  m_stream >> term;
  Q_ASSERT(term == fieldTerminator);
  return fieldInfo;
}

FieldSource::FieldSource(QFile* file, const QString& path)
  : m_stream(file)
  , m_attfLevel(LexLevel::N_A)
  , m_natfLevel(LexLevel::N_A)
{
  // read leader
  m_stream.skipRawData(5); // reclen
  auto magic = read_string(m_stream, 7);
  if (magic != "3LE1 09") {
    throw ChartFileError(QString("%1 is not a proper S57 chart file").arg(path));
  }
  auto headerlen = read_integer<quint32>(m_stream, 5);
  m_stream.skipRawData(3); // " ! "
  auto fieldLenFieldlen = read_integer<quint8>(m_stream, 1);
  auto fieldPosFieldlen = read_integer<quint8>(m_stream, 1);
  m_stream.skipRawData(2); // "04"
  // read directory
  FieldInfoVector fieldInfo = readDirectory(headerlen, fieldLenFieldlen, fieldPosFieldlen);
  // read field control field
  FieldInfo info = fieldInfo.takeFirst();
  m_stream.skipRawData(10); // "0000;&-A ", 0x1f
  int numTagPairs = (info.len - 11) / 8;
  for (int i = 0; i < numTagPairs; i++) {
    m_tags << read_string(m_stream, 4) << read_string(m_stream, 4);
  }
  quint8 term;
  m_stream >> term;
  Q_ASSERT(term == fieldTerminator);
  // read data descriptive fields
  for (int i = 0; i < fieldInfo.size(); i++) {
    m_stream.skipRawData(1); // structureCode
    auto typeCode = read_integer<quint8>(m_stream, 1);
    Q_ASSERT(typeCode == 5 || typeCode == 6); // mixed or binary
    m_stream.skipRawData(7); // "00;&-A "
    int len;
    read_bytes_until(m_stream, unitTerminator, len); // fieldName
    read_bytes_until(m_stream, unitTerminator, len); // arrayDescr
    read_bytes_until(m_stream, fieldTerminator, len); // fmtControls
  }
}

using HandlerFunc = std::function<bool (const S57::Record*)>;

struct Handler {
  Handler(HandlerFunc f): func(std::move(f)) {}
  HandlerFunc func;
  ~Handler() = default;
};



using Region = S57ChartOutline::Region;
using RT = S57::Record::Type::palette;
using Purp = S57::Record::ExPurp::palette;
using Topo = S57::Record::Topology::palette;
using Units = S57::Record::Units::palette;
using Instr = S57::Record::UpdateInstr::palette;
using Orient = S57::Record::Orient::palette;
using TopInd = S57::Record::TopInd::palette;
using Boundary = S57::Record::Boundary::palette;
using Geom = S57::Record::Geometry::palette;

GeoProjection* S57Reader::configuredProjection(const QString& path) const {

  QFile file(path);
  if (!file.open(QFile::ReadOnly)) {
    throw ChartFileError(QString("Cannot open %1 for reading").arg(path));
  }

  FieldSource source(&file, path);

  WGS84Point ref;
  quint32 mulfac;

  const QMap<RT, Handler*> handlers {


    {RT::DS, new Handler([] (const S57::Record*) {
        return true;
      })
    },

    {RT::DP, new Handler([&mulfac] (const S57::Record* rec) {
        auto dspm = dynamic_cast<const S57::DSPM*>(rec);
        mulfac = dspm->coordFactor;
        return true;
      })
    },

    {RT::VI, new Handler([&ref, &mulfac] (const S57::Record* rec) {
        auto vrid = dynamic_cast<const S57::VRID*>(rec);
        if (vrid->instruction.value != Instr::Insert) {
          return true;
        }
        auto sg2d = dynamic_cast<const S57::SG2D*>(vrid->find("SG2D"));
        if (sg2d != nullptr && !sg2d->points.isEmpty()) {
          const auto p0 = sg2d->points.first() / mulfac;
          ref = WGS84Point::fromLL(p0.x(), p0.y());
          return false;
        }
        auto sg3d = dynamic_cast<const S57::SG3D*>(vrid->find("SG3D"));
        if (sg3d != nullptr && !sg3d->soundings.isEmpty()) {
          const auto p0 = sg3d->soundings.first().location / mulfac;
          ref = WGS84Point::fromLL(p0.x(), p0.y());
          return false;
        }
        return true;
      })
    },

    {RT::VC, new Handler([&ref, &mulfac] (const S57::Record* rec) {
        auto vrid = dynamic_cast<const S57::VRID*>(rec);
        if (vrid->instruction.value != Instr::Insert) {
          return true;
        }
        auto sg2d = dynamic_cast<const S57::SG2D*>(vrid->find("SG2D"));
        const auto p0 = sg2d->points.first() / mulfac;
        ref = WGS84Point::fromLL(p0.x(), p0.y());
        return false;
      })
    },

    {RT::VE, new Handler([] (const S57::Record*) {
        return true;
      })
    },

    {RT::FE, new Handler([] (const S57::Record*) {
        return true;
      })
    },


  };

  bool done = false;

  while (!done) {
    S57::Record* rec = source.nextRecord();
    if (!rec) {
      done = true;
    } else {
      if (!handlers.contains(rec->type().value)) {
        done = true;
      } else {
        done = !(handlers[rec->type().value])->func(rec);
      }
      delete rec;
    }
  }

  qDeleteAll(handlers);
  if (!ref.valid()) {
    throw ChartFileError(QString("Invalid S57 header in %1").arg(path));
  }

  auto gp = GeoProjection::CreateProjection(m_proj->className());
  gp->setReference(ref);
  return gp;
}


S57ChartOutline S57Reader::readOutline(const QString& path, const GeoProjection* gp) const {

  QDate pub;
  QDate mod;
  quint32 scale = 0;
  quint32 mulfac = 0;

  const quint32 m_covr = S52::FindCIndex("M_COVR");
  const quint32 catcov = S52::FindCIndex("CATCOV");
  bool inCov = false;

  RawEdgeMap edges;
  PointMap connected;
  CovEdgeRefMap covRefs;

  const QMap<RT, Handler*> handlers {

    {RT::DS, new Handler([&pub, &mod] (const S57::Record* rec) {
        auto dsid = dynamic_cast<const S57::DSID*>(rec);
        if (dsid->issued.isValid()) {
          mod = dsid->issued;
        }
        if (dsid->updated.isValid()) {
          pub = dsid->updated;
        }
        return true;
      })
    },

    {RT::DP, new Handler([&scale, &mulfac] (const S57::Record* rec) {
        auto dspm = dynamic_cast<const S57::DSPM*>(rec);
        if (dspm->scale != 0) {
          scale = dspm->scale;
        }
        if (dspm->coordFactor != 0) {
          mulfac = dspm->coordFactor;
        }
        return true;
      })
    },

    {RT::VI, new Handler([] (const S57::Record*) {
        // skip isolated points
        return true;
      })
    },

    {RT::VC, new Handler([&connected] (const S57::Record* rec) {
        auto vrid = dynamic_cast<const S57::VRID*>(rec);
        if (vrid->instruction.value == Instr::Delete) {
          // qCDebug(CENC) << "removing connected node" << vrid->id();
          connected.remove(vrid->id());
          return true;
        }
        if (vrid->instruction.value == Instr::Insert) {
          auto sg2d = dynamic_cast<const S57::SG2D*>(vrid->find("SG2D"));
          connected[vrid->id()] = sg2d->points.first();
          return true;
        }
        // Modify: TODO
        Q_ASSERT(false);
        return false;
      })
    },

    {RT::VE, new Handler([&edges] (const S57::Record* rec) {
        auto vrid = dynamic_cast<const S57::VRID*>(rec);
        if (vrid->instruction.value == Instr::Delete) {
          // qCDebug(CENC) << "removing edge" << vrid->id();
          edges.remove(vrid->id());
          return true;
        }

        if (vrid->instruction.value == Instr::Insert) {
          auto vrpt = dynamic_cast<const S57::VRPT*>(vrid->find("VRPT"));
          if (vrpt->pointers.size() != 2) {
            return false;
          }
          RawEdge edge;
          for (const S57::VRPT::PointerField& pf: vrpt->pointers) {
            if (pf.type.value != RT::VC || pf.orient.value != Orient::N_A) {
              return false;
            }
            if (pf.topind.value == TopInd::Begin) {
              edge.begin = pf.id;
            } else if (pf.topind.value == TopInd::End) {
              edge.end = pf.id;
            } else {
              qCDebug(CENC) << "unhandled edge topology indicator" << pf.topind.print();
              return false;
            }
          }
          auto sg2d = dynamic_cast<const S57::SG2D*>(vrpt->find("SG2D"));
          if (sg2d != nullptr) {
            edge.points = sg2d->points;
          }
          edges[vrid->id()] = edge;
          return true;
        }
        // Modify
        auto vrpc = dynamic_cast<const S57::VRPC*>(vrid->find("VRPC"));
        if (vrpc != nullptr) {
          auto vrpt = dynamic_cast<const S57::VRPT*>(vrid->find("VRPT"));
          Q_ASSERT(vrpc->instruction.value == Instr::Modify); // only modify makes sense
          for (int i = 0; i < vrpc->count; i++) {
            const S57::VRPT::PointerField pf = vrpt->pointers[i];
            if (pf.topind.value == TopInd::Begin) {
              qCDebug(CENC) << "replacing begin" << edges[vrid->id()].begin << pf.id;
              edges[vrid->id()].begin = pf.id;
            } else if (pf.topind.value == TopInd::End) {
              qCDebug(CENC) << "replacing end" << edges[vrid->id()].end << pf.id;
              edges[vrid->id()].end = pf.id;
            } else {
              qCDebug(CENC) << "unhandled edge topology indicator" << pf.topind.print();
              return false;
            }
          }
        }
        auto sgcc = dynamic_cast<const S57::SGCC*>(vrid->find("SGCC"));
        if (sgcc != nullptr) {
          if (sgcc->instruction.value == Instr::Delete) {
            edges[vrid->id()].points.remove(sgcc->first - 1, sgcc->count);
            return true;
          }
          auto sg2d = dynamic_cast<const S57::SG2D*>(vrid->find("SG2D"));
          if (sgcc->instruction.value == Instr::Insert) {
            for (int i = 0; i < sgcc->count; i++) {
              edges[vrid->id()].points.insert(sgcc->first - 1, sg2d->points[sgcc->count - 1 - i]);
            }
            return true;
          }
          // Modify
          for (int i = 0; i < sgcc->count; i++) {
            edges[vrid->id()].points[sgcc->first - 1 + i] = sg2d->points[i];
          }
        }
        return true;
      })
    },

    {RT::FE, new Handler([&inCov, &covRefs, m_covr, catcov] (const S57::Record* rec) {
        auto frid = dynamic_cast<const S57::FRID*>(rec);
        if (frid->code != m_covr) {
          return !inCov;
        }
        inCov = true;
        if (frid->instruction.value == Instr::Delete) {
          covRefs.remove(frid->id());
          return true;
        }
        if (frid->instruction.value == Instr::Insert) {
          CovRefs crefs;
          auto attf = dynamic_cast<const S57::ATTF*>(frid->find("ATTF"));
          // coverage / no coverage
          crefs.cov = attf->attributes[catcov].value().toUInt() == 1;
          auto fspt = dynamic_cast<const S57::FSPT*>(frid->find("FSPT"));
          for (const S57::FSPT::PointerField& pf: fspt->pointers) {
            RawEdgeRef ref;
            ref.id = pf.id;
            ref.reversed = pf.orient.value == Orient::Reverse;
            ref.inner = pf.boundary.value == Boundary::Interior;
            crefs.refs.append(ref);
          }
          covRefs[frid->id()] = crefs;
          return true;
        }
        // Modify
        auto fspc = dynamic_cast<const S57::FSPC*>(frid->find("FSPC"));
        if (fspc->instruction.value == Instr::Delete) {
          covRefs[frid->id()].refs.remove(fspc->first - 1, fspc->count);
          return true;
        }
        auto fspt = dynamic_cast<const S57::FSPT*>(frid->find("FSPT"));
        if (fspc->instruction.value == Instr::Insert) {
          for (int i = 0; i < fspc->count; i++) {
            auto pf = fspt->pointers[fspc->count - 1 - i];
            RawEdgeRef ref;
            ref.id = pf.id;
            ref.reversed = pf.orient.value == Orient::Reverse;
            ref.inner = pf.boundary.value == Boundary::Interior;
            covRefs[frid->id()].refs.insert(fspc->first - 1, ref);
          }
          return true;
        }
        // Modify
        for (int i = 0; i < fspc->count; i++) {
          auto pf = fspt->pointers[i];
          RawEdgeRef ref;
          ref.id = pf.id;
          ref.reversed = pf.orient.value == Orient::Reverse;
          ref.inner = pf.boundary.value == Boundary::Interior;
          covRefs[frid->id()].refs[fspc->first - 1 + i] = ref;
        }
        return true;
      })
    },

  };

  QFileInfo info(path);
  QString filter = QString("%1.[0-9][0-9][0-9]").arg(info.baseName());
  QDir dir(info.path());
  auto updates = dir.entryList(QStringList {filter}, QDir::Files | QDir::Readable, QDir::Name);
  if (updates.isEmpty()) {
    throw ChartFileError(QString("%1 not found").arg(path));
  }

  for (const QString& update: updates) {

    // qCDebug(CENC) << update;

    const QString updpth = dir.absoluteFilePath(update);
    QFile file(updpth);
    if (!file.open(QFile::ReadOnly)) {
      throw ChartFileError(QString("Cannot open %1 for reading").arg(updpth));
    }

    FieldSource source(&file, updpth);


    bool done = false;
    inCov = false;

    while (!done) {
      S57::Record* rec = source.nextRecord();
      if (!rec) {
        done = true;
      } else {
        if (!handlers.contains(rec->type().value)) {
          done = true;
        } else {
          done = !(handlers[rec->type().value])->func(rec);
        }
        delete rec;
      }
    }
  }

  qDeleteAll(handlers);

  RawEdgeRefVector cv;
  for (CovEdgeRefIter it = covRefs.cbegin(); it != covRefs.cend(); ++it) {
    // qCDebug(CENC) << "Coverage" << it.key() << it.value().cov;
    if (it.value().cov) {
      cv.append(it.value().refs);
    }
  }


  PRegion pcov;
  PRegion pnocov;
  createCoverage(pcov, pnocov, cv, edges, connected, mulfac, gp);
  // qCDebug(CENC) << "nocov areas" << pnocov.size();
  // qCDebug(CENC) << "cov areas" << pcov.size();

  // qCDebug(CENC) << pub << mod << scale;

  if (!pub.isValid() || !mod.isValid() || scale == 0 || pcov.isEmpty()) {
    throw ChartFileError(QString("Invalid S57 header in %1").arg(path));
  }
  Region nocov = transformCoverage(pnocov, gp, nullptr);
  WGS84Point corners[2];
  Region cov = transformCoverage(pcov, gp, corners);

  return S57ChartOutline(corners[0], corners[1], cov, nocov, scale, pub, mod);
}



namespace S57 {

// Helper class to set Object's private data
class ObjectBuilder {
public:
  void s57SetAttributes(S57::Object* obj, const AttributeMap& attrs) const {
    obj->m_attributes = attrs;
  }
  void s57SetGeometry(S57::Object* obj, S57::Geometry::Base* geom, const QRectF& bbox) const {
    obj->m_geometry = geom;
    obj->m_bbox = bbox;
  }
};

}

void S57Reader::readChart(GL::VertexVector& vertices,
                          GL::IndexVector& indices,
                          S57::ObjectVector& objects,
                          const QString& path,
                          const GeoProjection* gp) const {

  quint32 mulfac = 0;

  PointMap isolated;
  PointMap connected;
  RawEdgeMap edges;
  RawObjectMap features;

  FieldSource* currentSource = nullptr;

  using SoundingVector = QVector<S57::SG3D::Sounding>;
  using SoundingMap = QMap<quint32, SoundingVector>;

  SoundingMap soundings;

  const QMap<RT, Handler*> handlers {

    {RT::DS, new Handler([&currentSource] (const S57::Record* rec) {
        auto dssi = dynamic_cast<const S57::DSSI*>(rec->find("DSSI"));
        if (dssi != nullptr) {
          currentSource->setLexicalLevels(dssi->attfLevel.value, dssi->natfLevel.value);
        }
        return true;
      })
    },

    {RT::DP, new Handler([&mulfac] (const S57::Record* rec) {
        auto dspm = dynamic_cast<const S57::DSPM*>(rec);
        if (dspm->coordFactor != 0) {
          mulfac = dspm->coordFactor;
        }
        return true;
      })
    },

    {RT::VI, new Handler([&isolated, &soundings] (const S57::Record* rec) {
        auto vrid = dynamic_cast<const S57::VRID*>(rec);
        if (vrid->instruction.value == Instr::Delete) {
          if (isolated.contains(vrid->id())) {
            // qCDebug(CENC) << "removing isolated node" << vrid->id();
            isolated.remove(vrid->id());
            return true;
          }
          if (soundings.contains(vrid->id())) {
            // qCDebug(CENC) << "removing soundings node" << vrid->id();
            soundings.remove(vrid->id());
            return true;
          }
          Q_ASSERT(false);
        }
        if (vrid->instruction.value == Instr::Insert) {
          auto sg2d = dynamic_cast<const S57::SG2D*>(vrid->find("SG2D"));
          if (sg2d != nullptr) {
            Q_ASSERT(sg2d->points.size() == 1);
            isolated[vrid->id()] = sg2d->points.first();
            return true;
          }
          auto sg3d = dynamic_cast<const S57::SG3D*>(vrid->find("SG3D"));
          if (sg3d != nullptr) {
            // qCDebug(CENC) << "inserting soundings node" << vrid->id();
            soundings[vrid->id()] = sg3d->soundings;
            return true;
          }
          Q_ASSERT(false);
        }
        // Modify
        auto sgcc = dynamic_cast<const S57::SGCC*>(vrid->find("SGCC"));
        if (sgcc == nullptr) {
          // cannot continue without sgcc
          qCWarning(CENC) << "SGCC not found" << rec->records();
          return true;
        }
        if (sgcc->instruction.value == Instr::Delete) {
          Q_ASSERT(soundings.contains(vrid->id()));
          // qCDebug(CENC) << "removing soundings nodes" << vrid->id();
          soundings[vrid->id()].remove(sgcc->first - 1, sgcc->count);
          return true;
        }
        if (sgcc->instruction.value == Instr::Insert) {
          Q_ASSERT(soundings.contains(vrid->id()));
          // qCDebug(CENC) << "inserting soundings nodes" << vrid->id();
          auto sg3d = dynamic_cast<const S57::SG3D*>(vrid->find("SG3D"));
          for (int i = 0; i < sgcc->count; i++) {
            soundings[vrid->id()].insert(sgcc->first - 1, sg3d->soundings[sgcc->count - 1 - i]);
          }
          return true;
        }
        // Modify
        if (soundings.contains(vrid->id())) {
          auto sg3d = dynamic_cast<const S57::SG3D*>(vrid->find("SG3D"));
          for (int i = 0; i < sgcc->count; i++) {
            soundings[vrid->id()][sgcc->first - 1 + i] = sg3d->soundings[i];
          }
        } else {
          Q_ASSERT(sgcc->first == 1 && sgcc->count == 1);
          auto sg2d = dynamic_cast<const S57::SG2D*>(vrid->find("SG2D"));
          isolated[vrid->id()] = sg2d->points.first();
        }
        return true;
      })
    },

    {RT::VC, new Handler([&connected] (const S57::Record* rec) {
        auto vrid = dynamic_cast<const S57::VRID*>(rec);
        if (vrid->instruction.value == Instr::Delete) {
          // qCDebug(CENC) << "removing connected node" << vrid->id();
          connected.remove(vrid->id());
          return true;
        }
        if (vrid->instruction.value == Instr::Insert) {
          auto sg2d = dynamic_cast<const S57::SG2D*>(vrid->find("SG2D"));
          Q_ASSERT(sg2d->points.size() == 1);
          connected[vrid->id()] = sg2d->points.first();
          return true;
        }
        // Modify: TODO
        Q_ASSERT(false);
        return true;
      })
    },

    {RT::VE, new Handler([&edges] (const S57::Record* rec) {
        auto vrid = dynamic_cast<const S57::VRID*>(rec);
        if (vrid->instruction.value == Instr::Delete) {
          // qCDebug(CENC) << "removing edge" << vrid->id();
          edges.remove(vrid->id());
          return true;
        }

        if (vrid->instruction.value == Instr::Insert) {
          auto vrpt = dynamic_cast<const S57::VRPT*>(vrid->find("VRPT"));
          if (vrpt->pointers.size() != 2) {
            return false;
          }
          RawEdge edge;
          for (const S57::VRPT::PointerField& pf: vrpt->pointers) {
            if (pf.type.value != RT::VC || pf.orient.value != Orient::N_A) {
              return false;
            }
            if (pf.topind.value == TopInd::Begin) {
              edge.begin = pf.id;
            } else if (pf.topind.value == TopInd::End) {
              edge.end = pf.id;
            } else {
              qCDebug(CENC) << "unhandled edge topology indicator" << pf.topind.print();
              return false;
            }
          }
          auto sg2d = dynamic_cast<const S57::SG2D*>(vrpt->find("SG2D"));
          if (sg2d != nullptr) {
            edge.points = sg2d->points;
          }
          edges[vrid->id()] = edge;
          return true;
        }
        // Modify
        auto vrpc = dynamic_cast<const S57::VRPC*>(vrid->find("VRPC"));
        if (vrpc != nullptr) {
          auto vrpt = dynamic_cast<const S57::VRPT*>(vrid->find("VRPT"));
          Q_ASSERT(vrpc->instruction.value == Instr::Modify); // only modify makes sense
          for (int i = 0; i < vrpc->count; i++) {
            const S57::VRPT::PointerField pf = vrpt->pointers[i];
            if (pf.topind.value == TopInd::Begin) {
              // qCDebug(CENC) << "replacing begin" << edges[vrid->id()].begin << pf.id;
              edges[vrid->id()].begin = pf.id;
            } else if (pf.topind.value == TopInd::End) {
              // qCDebug(CENC) << "replacing end" << edges[vrid->id()].end << pf.id;
              edges[vrid->id()].end = pf.id;
            } else {
              qCDebug(CENC) << "unhandled edge topology indicator" << pf.topind.print();
              return false;
            }
          }
        }
        auto sgcc = dynamic_cast<const S57::SGCC*>(vrid->find("SGCC"));
        if (sgcc != nullptr) {
          if (sgcc->instruction.value == Instr::Delete) {
            edges[vrid->id()].points.remove(sgcc->first - 1, sgcc->count);
            return true;
          }
          auto sg2d = dynamic_cast<const S57::SG2D*>(vrid->find("SG2D"));
          if (sgcc->instruction.value == Instr::Insert) {
            for (int i = 0; i < sgcc->count; i++) {
              edges[vrid->id()].points.insert(sgcc->first - 1, sg2d->points[sgcc->count - 1 - i]);
            }
            return true;
          }
          // Modify
          for (int i = 0; i < sgcc->count; i++) {
            edges[vrid->id()].points[sgcc->first - 1 + i] = sg2d->points[i];
          }
        }
        return true;
      })
    },

    {RT::FE, new Handler([&features] (const S57::Record* rec) {
        auto frid = dynamic_cast<const S57::FRID*>(rec);
        if (frid->instruction.value == Instr::Delete) {
          features.remove(frid->id());
          return true;
        }
        if (frid->instruction.value == Instr::Insert) {
          RawObject feature;
          feature.geometry = static_cast<quint8>(frid->geom.value);
          feature.code = frid->code;

          auto attf = dynamic_cast<const S57::ATTF*>(frid->find("ATTF"));
          if (attf != nullptr) {
            for (auto it = attf->attributes.cbegin(); it != attf->attributes.cend(); ++it) {
              feature.attributes.insert(it.key(), it.value());
            }
          }
          auto natf = dynamic_cast<const S57::NATF*>(frid->find("NATF"));
          if (natf != nullptr) {
            for (auto it = natf->attributes.cbegin(); it != natf->attributes.cend(); ++it) {
              feature.attributes.insert(it.key(), it.value());
            }
          }

          auto fspt = dynamic_cast<const S57::FSPT*>(frid->find("FSPT"));
          if (fspt != nullptr) {
            auto t = fspt->pointers.first().type.value;
            feature.refType = static_cast<quint8>(t);
            if (t != RT::VE) { // VI or VC
              Q_ASSERT(fspt->pointers.size() == 1);
              feature.pointRef = fspt->pointers.first().id;
            } else {
              for (const S57::FSPT::PointerField& pf: fspt->pointers) {
                RawEdgeRef ref;
                ref.id = pf.id;
                ref.reversed = pf.orient.value == Orient::Reverse;
                ref.inner = pf.boundary.value == Boundary::Interior;
                feature.edgeRefs.append(ref);
              }
            }
          }

          features[frid->id()] = feature;
          return true;
        }
        // Modify
        Q_ASSERT(features.contains(frid->id()));

        S57::AttributeMap attributes;
        auto attf = dynamic_cast<const S57::ATTF*>(frid->find("ATTF"));
        if (attf != nullptr) {
          for (auto it = attf->attributes.cbegin(); it != attf->attributes.cend(); ++it) {
            attributes.insert(it.key(), it.value());
          }
        }
        auto natf = dynamic_cast<const S57::NATF*>(frid->find("NATF"));
        if (natf != nullptr) {
          for (auto it = natf->attributes.cbegin(); it != natf->attributes.cend(); ++it) {
            attributes.insert(it.key(), it.value());
          }
        }
        for (S57::AttributeIterator it = attributes.cbegin(); it != attributes.cend(); ++it) {
          if (it.value().type() == S57::AttributeType::Deleted) {
            features[frid->id()].attributes.remove(it.key());
          } else {
            features[frid->id()].attributes[it.key()] = it.value();
          }
        }

        auto fspc = dynamic_cast<const S57::FSPC*>(frid->find("FSPC"));
        if (fspc == nullptr) {
          // cannot modify without FSPC
          return true;
        }
        if (fspc->instruction.value == Instr::Delete) {
          auto t = static_cast<RT>(features[frid->id()].refType);
          if (t == RT::VE) {
            features[frid->id()].edgeRefs.remove(fspc->first - 1, fspc->count);
          } else {
            Q_ASSERT(fspc->count == 1 && fspc->first == 1);
            features[frid->id()].pointRef = 0;
          }
          return true;
        }
        auto fspt = dynamic_cast<const S57::FSPT*>(frid->find("FSPT"));
        if (fspc->instruction.value == Instr::Insert) {
          auto t = static_cast<RT>(features[frid->id()].refType);
          Q_ASSERT(t == RT::VE);
          for (int i = 0; i < fspc->count; i++) {
            auto pf = fspt->pointers[fspc->count - 1 - i];
            RawEdgeRef ref;
            ref.id = pf.id;
            ref.reversed = pf.orient.value == Orient::Reverse;
            ref.inner = pf.boundary.value == Boundary::Interior;
            features[frid->id()].edgeRefs.insert(fspc->first - 1, ref);
          }
          return true;
        }
        // Modify
        auto t = static_cast<RT>(features[frid->id()].refType);

        if (t != RT::VE) { // VI or VC
          Q_ASSERT(fspc->count == 1 && fspc->first == 1);
          features[frid->id()].pointRef = fspt->pointers.first().id;
        } else {
          for (int i = 0; i < fspc->count; i++) {
            auto pf = fspt->pointers[i];
            RawEdgeRef ref;
            ref.id = pf.id;
            ref.reversed = pf.orient.value == Orient::Reverse;
            ref.inner = pf.boundary.value == Boundary::Interior;
            features[frid->id()].edgeRefs[fspc->first - 1 + i] = ref;
          }
        }
        return true;
      })
    },

  };

  QFileInfo info(path);
  QString filter = QString("%1.[0-9][0-9][0-9]").arg(info.baseName());
  QDir dir(info.path());
  auto updates = dir.entryList(QStringList {filter}, QDir::Files | QDir::Readable, QDir::Name);
  if (updates.isEmpty()) {
    throw ChartFileError(QString("%1 not found").arg(path));
  }

  for (const QString& update: updates) {

    qCDebug(CENC) << update;

    const QString updpth = dir.absoluteFilePath(update);
    QFile file(updpth);
    if (!file.open(QFile::ReadOnly)) {
      throw ChartFileError(QString("Cannot open %1 for reading").arg(updpth));
    }

    FieldSource source(&file, updpth);

    bool done = false;
    currentSource = &source;

    while (!done) {
      S57::Record* rec = source.nextRecord();
      if (!rec) {
        done = true;
      } else {
        if (!handlers.contains(rec->type().value)) {
          done = true;
        } else {
          done = !(handlers[rec->type().value])->func(rec);
        }
        delete rec;
      }
    }
  }
  qDeleteAll(handlers);

  S57::ObjectBuilder helper;

  auto getIso = [isolated, mulfac, gp] (quint32 id) {
    const QPointF ll = isolated[id] / mulfac;
    return gp->fromWGS84(WGS84Point::fromLL(ll.x(), ll.y()));
  };

  auto getConn = [connected, mulfac, gp] (quint32 id) {
    const QPointF ll = connected[id] / mulfac;
    return gp->fromWGS84(WGS84Point::fromLL(ll.x(), ll.y()));
  };

  auto getSnd = [soundings, mulfac, gp] (quint32 id) {
    GL::VertexVector ps;
    for (const S57::SG3D::Sounding& s: soundings[id]) {
      const QPointF ll = s.location / mulfac;
      auto p = gp->fromWGS84(WGS84Point::fromLL(ll.x(), ll.y()));
      ps << p.x() << p.y() << .1 * s.value; // sounding factor = 10
    }
    return ps;
  };

  // fill in vertices from connected points and edges
  PointRefMap pconn;
  for (PointMapIter it = connected.cbegin(); it != connected.cend(); ++it) {
    pconn[it.key()] = vertices.size() / 2;
    auto p = getConn(it.key());
    vertices << p.x() << p.y();
  }

  EdgeMap pedges;
  for (REMIter it = edges.cbegin(); it != edges.cend(); ++it) {
    const RawEdge& r = it.value();
    Edge e;
    e.begin = pconn[r.begin];
    e.end = pconn[r.end];
    e.first = vertices.size() / 2;
    PointVector ps;
    ps << QPointF(vertices[2 * e.begin], vertices[2 * e.begin + 1]);
    for (const QPointF& q: r.points) {
      const QPointF ll = q / mulfac;
      ps.append(gp->fromWGS84(WGS84Point::fromLL(ll.x(), ll.y())));
    }
    ps << QPointF(vertices[2 * e.end], vertices[2 * e.end + 1]);
    IndexVector reduced;
    reduceRDP(ps, 0, ps.size() - 1, reduced);
    if (reduced.size() < ps.size() - 2) {
      // qCDebug(CENC) << "RDP reduction" << ps.size() - 2 << " -> " << reduced.size();
      std::sort(reduced.begin(), reduced.end());
      for (auto i: reduced) {
        auto const p = ps[i];
        vertices << p.x() << p.y();
      }
      e.count = reduced.size();
    } else {
      for (int i = 1; i < ps.size() - 1; ++i) {
        auto const p = ps[i];
        vertices << p.x() << p.y();
      }
      e.count = ps.size() - 2;
    }
    pedges[it.key()] = e;
  }
  // not needed anymore
  edges.clear();

  for (ROMIter it = features.cbegin(); it != features.cend(); ++it) {
    const RawObject& feature = it.value();
    auto obj = new S57::Object(it.key(), feature.code);
    objects.append(obj);

    helper.s57SetAttributes(obj, feature.attributes);

    auto geom = static_cast<Geom>(feature.geometry);
    if (geom == Geom::Point) {
      auto t = static_cast<RT>(feature.refType);
      if (t == RT::VI) {
        if (isolated.contains(feature.pointRef)) {
          auto p = getIso(feature.pointRef);
          QRectF bbox(p - QPointF(10, 10), QSizeF(20, 20));
          helper.s57SetGeometry(obj, new S57::Geometry::Point(p, gp), bbox);
        } else if (soundings.contains(feature.pointRef)) {
          auto ps = getSnd(feature.pointRef);
          auto bbox = computeSoundingsBBox(ps);
          helper.s57SetGeometry(obj, new S57::Geometry::Point(ps, gp), bbox);
        } else {
          Q_ASSERT(false);
        }
      } else if (t == RT::VC) {
        Q_ASSERT(connected.contains(feature.pointRef));
        auto p = getConn(feature.pointRef);
        QRectF bbox(p - QPointF(10, 10), QSizeF(20, 20));
        helper.s57SetGeometry(obj, new S57::Geometry::Point(p, gp), bbox);
      }
    } else if (geom == Geom::Line || geom == Geom::Area) {
      EdgeVector shape;
      for (const RawEdgeRef& ref: feature.edgeRefs) {
        Edge e = pedges[ref.id];
        e.reversed = ref.reversed;
        e.inner = ref.inner;
        shape.append(e);
      }
      auto lines = createLineElements(indices, vertices, shape);
      auto bbox = computeBBox(lines, vertices, indices);

      if (geom == Geom::Area) {
        S57::ElementDataVector triangles;
        triangulate(triangles, indices, vertices, lines);

        auto center = computeAreaCenterAndBboxes(triangles, vertices, indices);
        helper.s57SetGeometry(obj,
                              new S57::Geometry::Area(lines,
                                                      center,
                                                      triangles,
                                                      0,
                                                      true,
                                                      gp),
                              bbox);

      } else {
        auto center = computeLineCenter(lines, vertices, indices);
        helper.s57SetGeometry(obj,
                              new S57::Geometry::Line(lines, center, 0, gp),
                              bbox);
      }

    } else {
      helper.s57SetGeometry(obj, new S57::Geometry::Meta(), QRectF());
    }
  }

}

void S57Reader::createCoverage(PRegion &cov, PRegion &nocov,
                               const RawEdgeRefVector &edges,
                               const RawEdgeMap &edgemap,
                               const PointMap &connmap,
                               quint32 mulfac, const GeoProjection *gp) const {

  auto startPoint = [edges, edgemap] (int i) {
    return edges[i].reversed ? edgemap[edges[i].id].end : edgemap[edges[i].id].begin;
  };

  auto endPoint = [edges, edgemap] (int i) {
    return edges[i].reversed ? edgemap[edges[i].id].begin : edgemap[edges[i].id].end;
  };


  for (int i = 0; i < edges.size();) {
    PointVector ps;
    auto start = startPoint(i);
    auto prevlast = start;
    while (i < edges.size() && prevlast == startPoint(i)) {
      ps.append(addVertices(edges[i], edgemap, connmap, mulfac, gp));
      prevlast = endPoint(i);
      i++;
    }
    Q_ASSERT(prevlast == start);
    bool inner = edges[i - 1].inner;
    // qCDebug(CENC) << "Reducing" << cov.size() << "Inner" << inner;
    PointVector qs;
    const int N = ps.size();
    for (int k = 0; k < N; k++) {
      const QPointF v = ps[k];
      const QPointF vm = ps[(k + N - 1) % N];
      const QPointF vp = ps[(k + 1) % N];
      if (v.x() == vm.x() && v.x() == vp.x()) continue;
      if (v.y() == vm.y() && v.y() == vp.y()) continue;
      qs << v;
      // qCDebug(CENC) << v.x() << v.y();
    }

    if (inner) {
      nocov << qs;
    } else {
      cov << qs;
    }
  }
}

S57Reader::PointVector S57Reader::addVertices(const RawEdgeRef& e,
                                              const RawEdgeMap& edgemap,
                                              const PointMap& connmap,
                                              quint32 mulfac, const GeoProjection* gp) const {

  PointVector ps;

  auto getPoint = [edgemap, mulfac, gp] (quint32 id, int i) {
    const QPointF ll = edgemap[id].points[i] / mulfac;
    return gp->fromWGS84(WGS84Point::fromLL(ll.x(), ll.y()));
  };

  auto getBeginPoint = [edgemap, connmap, mulfac, gp] (quint32 id) {
    const QPointF ll = connmap[edgemap[id].begin] / mulfac;
    return gp->fromWGS84(WGS84Point::fromLL(ll.x(), ll.y()));
  };

  auto getEndPoint = [edgemap, connmap, mulfac, gp] (quint32 id) {
    const QPointF ll = connmap[edgemap[id].end] / mulfac;
    return gp->fromWGS84(WGS84Point::fromLL(ll.x(), ll.y()));
  };

  const int count = edgemap[e.id].points.size();

  if (!e.reversed) {
    ps << getBeginPoint(e.id);
    for (int i = 0; i < count; i++) {
      ps << getPoint(e.id, i);
    }
  } else {
    ps << getEndPoint(e.id);
    for (int i = 0; i < count; i++) {
      ps << getPoint(e.id, count - i - 1);
    }
  }
  return ps;
}

static float area(const QVector<QPointF>& ps) {
  float sum = 0;
  const int n = ps.size();
  for (int k = 0; k < n; k++) {
    const QPointF p0 = ps[k];
    const QPointF p1 = ps[(k + 1) % n];
    sum += p0.x() * p1.y() - p0.y() * p1.x();
  }
  return .5 * sum;
}


bool S57Reader::checkCoverage(const PRegion& cov,
                              WGS84Point &sw,
                              WGS84Point &ne,
                              const GeoProjection *gp) const {

  if (cov.isEmpty()) return false;

  // compute bounding box
  QPointF ur(-1.e15, -1.e15);
  QPointF ll(1.e15, 1.e15);
  for (const PointVector& ps: cov) {
    for (const QPointF& p: ps) {
      ur.setX(qMax(ur.x(), p.x()));
      ur.setY(qMax(ur.y(), p.y()));
      ll.setX(qMin(ll.x(), p.x()));
      ll.setY(qMin(ll.y(), p.y()));
    }
  }

  const float A = (ur.x() - ll.x()) * (ur.y() - ll.y());
  float totcov = 0;
  for (const PointVector& ps: cov) {
    totcov += std::abs(area(ps) / A);
  }
  sw = gp->toWGS84(ll);
  ne = gp->toWGS84(ur);
  if (sw == WGS84Point::fromLL(ne.lng(), sw.lat())) {
    qCDebug(CENC) << ll << ur;
    qCDebug(CENC) << sw.lng() << sw.lat() << ne.lng() << ne.lat();
    throw ChartFileError("Too narrow coverage");
  }
  if (sw == WGS84Point::fromLL(sw.lng(), ne.lat())) {
    qCDebug(CENC) << ll << ur;
    qCDebug(CENC) << sw.lng() << sw.lat() << ne.lng() << ne.lat();
    throw ChartFileError("Too low coverage");
  }
  // qCDebug(CENC) << "totcov" << totcov;
  return totcov < .8;
}

Region S57Reader::transformCoverage(PRegion pcov, const GeoProjection *gp, WGS84Point* corners) const {

  if (corners != nullptr && !checkCoverage(pcov, corners[0], corners[1], gp)) {
    return Region();
  }
  Region cov;
  for (const PointVector& ps: pcov) {
    WGS84PointVector ws;
    for (auto p: ps) {
      ws << gp->toWGS84(p);
    }
    cov.append(ws);
  }
  return cov;
}

// https://en.wikipedia.org/wiki/Ramer%E2%80%93Douglas%E2%80%93Peucker_algorithm
void S57Reader::reduceRDP(const PointVector& ps, int first, int last, IndexVector& is) const {
  // Find the point with the maximum distance
  double d2max = 0;
  int index = 0;

  const QPointF p = ps[last] - ps[first];

  auto p2 = QPointF::dotProduct(p, p);

  if (p2 < eps) {
    // closed loop
    for (int i = first + 1; i < last; ++i) {
      const QPointF q = ps[i] - ps[first];
      const auto q2 = QPointF::dotProduct(q, q);
      if (q2 > d2max) {
        index = i;
        d2max = q2;
      }
    }
  } else {
    for (int i = first + 1; i < last; ++i) {
      const QPointF q = ps[i] - ps[first];

      const auto qp = QPointF::dotProduct(q, p);
      const auto q2 = QPointF::dotProduct(q, q);

      const auto d2 = q2 - qp * qp / p2;
      if (d2 > d2max) {
        index = i;
        d2max = d2;
      }
    }
  }

  // If max distance is greater than epsilon, recursively simplify
  if (d2max > eps) {
    is << index;
    reduceRDP(ps, first, index, is);
    reduceRDP(ps, index, last, is);
  }

}



QString S57ReaderFactory::name() const {
  return "s57";
}

QString S57ReaderFactory::displayName() const {
  return "S57 Charts";
}

QStringList S57ReaderFactory::filters() const {
  return QStringList {"*.000"};
}

void S57ReaderFactory::initialize() const {
  // noop
}

ChartFileReader* S57ReaderFactory::create() const {
  return new S57Reader(name());
}


