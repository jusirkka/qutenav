/* -*- coding: utf-8-unix -*-
 *
 * File: src/hpglparser.h
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
#pragma once

#include <QString>
#include <QVector>
#include "types.h"
#include <QStack>

#define S52HPGL_LTYPE HPGL::LocationType
#define S52HPGL_STYPE HPGL::ValueType

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

namespace HPGL {

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

using RawPoints = QVector<int>;

class Parser {

  friend class ::HPGLHelper;

public:

  Parser(const QString& colors);

  bool ok() const {return m_ok;}

  virtual ~Parser() = default;

protected:

  using ColorRef = QMap<char, quint32>;

  void parseColorRef(const QString& cmap);
  bool m_ok;

  ColorRef m_cmap;

  void parse(const QString& src);

protected: // bison interface

  virtual void setColor(char c) = 0;
  virtual void setAlpha(int a) = 0;
  virtual void setWidth(int w) = 0;
  virtual void movePen(const RawPoints& ps) = 0;
  virtual void drawLineString(const RawPoints& ps) = 0;
  virtual void drawCircle(int r) = 0;
  virtual void drawArc(int x, int y, int a) = 0;
  virtual void pushSketch() = 0;
  virtual void endSketch() = 0;
  virtual void fillSketch() = 0;
  virtual void edgeSketch() = 0;
  virtual void drawPoint() = 0;


};
}

void s52hpgl_error(HPGL::LocationType*,
                   HPGL::Parser*,
                   yyscan_t, const char*);


