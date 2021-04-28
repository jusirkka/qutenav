/* -*- coding: utf-8-unix -*-
 *
 * File: src/wfreader.h
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

#include <QVector>
#include <QOpenGLFunctions>

#define WAVEFRONT_LTYPE WF::LocationType
#define WAVEFRONT_STYPE WF::ValueType

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
using yyscan_t = void *;
#endif


namespace WF {

struct LocationType {
  int row;
  int col;
  int pos;
  int prev_col;
  int prev_pos;
};

class TripletIndex {
public:
    TripletIndex(int v = 0, int t = 0, int n = 0)
        : v_index(v)
        , t_index(t)
        , n_index(n) {}

    int v_index;
    int t_index;
    int n_index;
};

using TripletIndexVector = QVector<TripletIndex>;

using IndexVector = QVector<int>;
using NumericVector = QVector<float>;

struct ValueType {
  int v_int;
  float v_float;
  int v_triplet[3];
  TripletIndexVector v_triplets;
  QString v_string;
  NumericVector v_floats;
  IndexVector v_ints;
};


class ModelError {

public:
  ModelError(QString msg, int row, int col, int pos)
    : emsg(std::move(msg))
    , erow(row)
    , ecol(col)
    , epos(pos)
  {}

  ModelError()
    :emsg(),
      erow(0),
      ecol(0),
      epos(0)
  {}

  const QString msg() const {return emsg;}
  int row() const {return erow;}
  int col() const {return ecol;}
  int pos() const {return epos;}

private:

  QString emsg;
  int erow;
  int ecol;
  int epos;

};

} // namespace WF

class WFReader
{
public:
  WFReader();

  void parse(const QString& path);
  void reset();

  using Indices = QVector<GLuint>;
  using Mesh = QVector<GLfloat>;

  const Indices& triangles() const {return m_triangles;}
  const Mesh& vertices() const {return m_vertices;}

  // Grammar interface
  void appendVertex(float x, float y, float z, float w = 1.);
  void appendFace(const WF::TripletIndexVector&);

  enum Error {InSurfDef, StateNotComplete, SurfDefRequired, Unused};
  void createError(const QString& item, Error err);

private:

  Indices m_triangles;
  Mesh m_vertices;

  WF::ModelError m_error;
  yyscan_t m_scanner;

};



#define YYLLOC_DEFAULT(Current, Rhs, N) do if (N) {\
    (Current).row = YYRHSLOC (Rhs, 1).row;\
    (Current).col = YYRHSLOC (Rhs, 1).col;\
    (Current).prev_col = YYRHSLOC (Rhs, 1).prev_col;\
    (Current).pos = YYRHSLOC (Rhs, 1).pos;\
    (Current).prev_pos = YYRHSLOC (Rhs, 1).prev_pos;\
    } else {                                        \
    (Current).row = YYRHSLOC (Rhs, 0).row;\
    (Current).col = YYRHSLOC (Rhs, 0).col;\
    (Current).prev_col = YYRHSLOC (Rhs, 0).prev_col;\
    (Current).pos = YYRHSLOC (Rhs, 0).pos;\
    (Current).prev_pos = YYRHSLOC (Rhs, 0).prev_pos;\
    } while (0)

void wavefront_error(WF::LocationType*, WFReader*, yyscan_t, const char*);

