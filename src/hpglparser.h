#pragma once

#include <QString>
#include <QVector>
#include "types.h"
#include <QStack>

#define S52HPGL_LTYPE HPGLParser::LocationType
#define S52HPGL_STYPE HPGLParser::ValueType

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
using yyscan_t = void *;
#endif

#define YYLLOC_DEFAULT(Current, Rhs, N) do if (N) {\
    (Current).pos = YYRHSLOC (Rhs, 1).pos;\
    (Current).prev_pos = YYRHSLOC (Rhs, 1).prev_pos;\
    } else {                                        \
    (Current).pos = YYRHSLOC (Rhs, 0).pos;\
    (Current).prev_pos = YYRHSLOC (Rhs, 0).prev_pos;\
    } while (0)



class HPGLHelper;

class HPGLParser {

  friend class HPGLHelper;

public:

  // flex/bison location and value types
  struct LocationType {
    int prev_pos;
    int pos;
  };

  struct ValueType {
    char v_char;
    int v_int;
    QVector<int> v_int_list;
  };


  HPGLParser(const QString& src, const QString& colors, const QPoint& pivot);

  using RawPoints = QVector<int>;

  struct Data {
    S52::Color color;
    GL::IndexVector indices;
    GL::VertexVector vertices;
  };

  using DataVector = QVector<Data>;

  const DataVector& data() const {return m_data;}
  bool ok() const {return m_ok;}

private:

  using ColorRef = QMap<char, quint32>;
  using DataHash = QHash<S52::Color, Data>;
  using DataIterator = QHash<S52::Color, Data>::iterator;

  void parseColorRef(const QString& cmap);
  bool m_ok;
  DataVector m_data;

  ColorRef m_cmap;

private: // bison interface

  const int line_width_multiplier = 20;


  void setColor(char c);
  void setAlpha(int a);
  void setWidth(int w);
  void movePen(const RawPoints& ps);
  void drawLineString(const RawPoints& ps);
  void drawCircle(int r);
  void drawArc(int x, int y, int a);
  void pushSketch();
  void endSketch();
  void fillSketch();
  void edgeSketch();
  void drawPoint();

  using PointList = QVector<QPointF>;

  struct LineString {
    PointList points;
    bool closed;
  };

  using LineStringList = QVector<LineString>;

  struct Sketch {
    inline Sketch clone() {
      Sketch s;
      s.color = color;
      s.lineWidth = lineWidth;
      LineString ls;
      ls.closed = false;
      s.parts.append(ls);
      return s;
    }
    S52::Color color;
    int lineWidth;
    LineStringList parts;
  };

  void triangulate(const PointList& points, Data& out);
  void thickerlines(const LineString& ls, int lw, Data& out);
  static void mergeData(Data& tgt, const Data& d);
  QPointF makePoint(int x, int y) const;

  Sketch m_currentSketch;
  QStack<Sketch> m_sketches;

  bool m_started;
  bool m_penDown;

  QPointF m_pen;
  QPoint m_pivot;

  DataHash m_storage;

};

void s52hpgl_error(HPGLParser::LocationType*,
                   HPGLParser*,
                   yyscan_t, const char*);


