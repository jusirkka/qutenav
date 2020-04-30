#include "wfreader.h"

#include "wavefront_parser.h"
#ifndef YYSTYPE
#define YYSTYPE WAVEFRONT_STYPE
#endif
#ifndef YYLTYPE
#define YYLTYPE WAVEFRONT_LTYPE
#endif
#include "wavefront_scanner.h"
#include "triangleoptimizer.h"

#include <QDebug>
#include <QFile>

WFReader::WFReader() {}

void WFReader::parse(const QString &path) {
  try {

    if (path.isEmpty()) return;

    QFile file(path);
    file.open(QFile::ReadOnly);
    auto src = QString(file.readAll()).append('\n');
    file.close();

    wavefront_lex_init(&m_scanner);
    wavefront__scan_string(src.toUtf8().data(), m_scanner);
    int err = wavefront_parse(this, m_scanner);
    wavefront_lex_destroy(m_scanner);

    if (err) throw WF::ModelError(m_error);

  } catch (WF::ModelError& e) {
    qWarning() << e.msg() << e.row() << e.col();
  }

  m_stripper = AC::TriangleOptimizer(m_triangles);


}

void WFReader::reset() {
  m_vertices.clear();
  m_triangles.clear();
}


void WFReader::appendVertex(float x, float y, float z, float) {
  m_vertices.append(x);
  m_vertices.append(y);
  m_vertices.append(z);
}

static GLuint makeIndex(int x, unsigned int l) {
  if (l == 0) return 0;
  if (x < 1) return (x + l) % l;
  return (x - 1) % l;
}


void WFReader::appendFace(const WF::TripletIndexVector& ts) {
  if (ts.size() != 3) return;

  unsigned int Lv = m_vertices.size() / 3;
  if (Lv == 0) {
    WF::LocationType* loc = wavefront_get_lloc(m_scanner);
    throw WF::ModelError("no vertices", loc->row, loc->col, loc->pos);
  }
  for (const WF::TripletIndex t: ts) {
    if (t.v_index == 0) {
      WF::LocationType* loc = wavefront_get_lloc(m_scanner);
      throw WF::ModelError("bad face data", loc->row, loc->col, loc->pos);
    }
    m_triangles.append(makeIndex(t.v_index, Lv));
  }
}

void WFReader::createError(const QString &msg, Error err) {
    QString detail;
    switch (err) {
    case InSurfDef:
        detail = "%1 cannot be used inside surface definition";
        break;
    case StateNotComplete:
        detail = "%1 cannot be used when surface state is not complete";
        break;
    case SurfDefRequired:
        detail = "%1 can only be used inside surface definition";
        break;
    default:
        detail = "%1";
    }
    WF::LocationType* loc = wavefront_get_lloc(m_scanner);
    m_error = WF::ModelError(detail.arg(msg), loc->row, loc->col, loc->pos);
}

void wavefront_error(WF::LocationType*, WFReader* reader, yyscan_t, const char* msg) {
  reader->createError(msg, WFReader::Error::Unused);
}
